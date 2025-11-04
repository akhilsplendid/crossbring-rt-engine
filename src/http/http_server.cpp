#ifdef USE_HTTP_SERVER

#include "crossbring/http/http_server.h"

#include <sstream>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "crossbring/sinks/recent_buffer_sink.h"
#include "crossbring/http/event_hub.h"

namespace crossbring {

static std::string metrics_text(Engine& engine) {
    std::ostringstream os;
    os << "# HELP crossbring_processed_total Total processed events\n";
    os << "# TYPE crossbring_processed_total counter\n";
    os << "crossbring_processed_total " << engine.processed_count() << "\n";
    os << "# HELP crossbring_dropped_total Total dropped events\n";
    os << "# TYPE crossbring_dropped_total counter\n";
    os << "crossbring_dropped_total " << engine.dropped_count() << "\n";
    return os.str();
}

HttpServer::HttpServer(Engine& engine, std::shared_ptr<RecentBuffer> recent, const std::string& host, int port, std::shared_ptr<EventHub> hub)
    : engine_(engine), recent_(std::move(recent)), hub_(std::move(hub)), host_(host), port_(port) {}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
    if (running_.exchange(true)) return;
    th_ = std::thread([this]{ run(); });
}

void HttpServer::stop() {
    if (!running_.exchange(false)) return;
    if (th_.joinable()) th_.join();
}

void HttpServer::run() {
    httplib::Server svr;

    svr.Get("/metrics", [this](const httplib::Request&, httplib::Response& res){
        auto txt = metrics_text(engine_);
        res.set_content(txt, "text/plain; version=0.0.4");
    });

    svr.Get("/recent", [this](const httplib::Request& req, httplib::Response& res){
        size_t n = 100;
        if (req.has_param("n")) {
            n = std::stoul(req.get_param_value("n"));
        }
        auto arr = recent_->snapshot_json(n);
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/", [](const httplib::Request&, httplib::Response& res){
        static const char* html = R"HTML(
<!doctype html>
<html><head><meta charset='utf-8'><title>Crossbring Dashboard</title>
<style>body{font-family:system-ui,Segoe UI,Arial;margin:20px} table{border-collapse:collapse} td,th{border:1px solid #ccc;padding:4px} .live{color:green;font-weight:bold}</style>
</head><body>
<h1>Crossbring RT Engine Dashboard</h1>
<p><a href="/metrics" target="_blank">/metrics</a> | <a href="/recent" target="_blank">/recent</a> | <span class="live" id="live">SSE: connecting...</span></p>
<div id="stats"></div>
<table id="tbl"><thead><tr><th>source</th><th>key</th><th>title/value</th></tr></thead><tbody></tbody></table>
<script>
async function refresh(){
  const rec = await fetch('/recent?n=50').then(r=>r.json());
  const tb = document.querySelector('#tbl tbody'); tb.innerHTML='';
  for(const e of rec.reverse()){
    const tr=document.createElement('tr');
    const title = e.payload?.title || e.payload?.value || '';
    tr.innerHTML=`<td>${e.source}</td><td>${e.key}</td><td>${String(title).slice(0,80)}</td>`;
    tb.appendChild(tr);
  }
  const met = await fetch('/metrics').then(r=>r.text());
  document.getElementById('stats').innerText = met;
}
setInterval(refresh, 2000); refresh();

// Live SSE stream
try {
  const es = new EventSource('/sse');
  es.onopen = () => { document.getElementById('live').textContent = 'SSE: connected'; };
  es.onmessage = (ev) => {
    try {
      const e = JSON.parse(ev.data);
      const tb = document.querySelector('#tbl tbody');
      const tr=document.createElement('tr');
      const title = e.payload?.title || e.payload?.value || '';
      tr.innerHTML=`<td>${e.source}</td><td>${e.key}</td><td>${String(title).slice(0,80)}</td>`;
      tb.insertBefore(tr, tb.firstChild);
      // trim table
      while (tb.children.length > 100) tb.removeChild(tb.lastChild);
    } catch {}
  };
  es.onerror = () => { document.getElementById('live').textContent = 'SSE: error'; };
} catch (e) { console.warn('SSE not supported', e); }
</script>
</body></html>
)HTML";
        res.set_content(html, "text/html");
    });

    // SSE endpoint using EventHub
    auto hub = hub_ ? hub_ : std::make_shared<EventHub>();
    // Attach sink so every event is published to SSE clients
    // Note: Engine reference is captured via 'this'; sinks are added outside in main.
    svr.Get("/sse", [hub](const httplib::Request&, httplib::Response& res){
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider("text/event-stream",
            [hub](size_t /*offset*/, httplib::DataSink& sink) {
                auto q = hub->register_consumer();
                while (true) {
                    nlohmann::json j;
                    if (!q->pop(j)) break;
                    std::string line = "data: " + j.dump() + "\n\n";
                    if (!sink.write(line.data(), line.size())) break;
                }
                return true;
            },
            [hub](bool) {
                // nothing; queues will be GC'ed when no reference
            }
        );
    });

    // Expose a simple info endpoint so main() can register the EventHub sink
    svr.Options("/sse", [](const httplib::Request&, httplib::Response& res){ res.status = 204; });

    svr.listen(host_.c_str(), port_);
}

} // namespace crossbring

#endif // USE_HTTP_SERVER
