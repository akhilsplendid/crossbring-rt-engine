#ifdef USE_SQLITE

#include <sqlite3.h>
#include <mutex>
#include <spdlog/spdlog.h>

#include "crossbring/sinks/sink.h"

namespace crossbring {

class SQLiteSink : public Sink {
public:
    explicit SQLiteSink(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite DB: " + path);
        }
        const char* ddl =
            "CREATE TABLE IF NOT EXISTS events (" \
            " id INTEGER PRIMARY KEY AUTOINCREMENT," \
            " ts_ns INTEGER," \
            " source TEXT," \
            " key TEXT," \
            " payload TEXT" \
            ");";
        char* err = nullptr;
        if (sqlite3_exec(db_, ddl, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "unknown";
            sqlite3_free(err);
            throw std::runtime_error("SQLite DDL error: " + msg);
        }
        const char* ins = "INSERT INTO events (ts_ns, source, key, payload) VALUES (?,?,?,?);";
        if (sqlite3_prepare_v2(db_, ins, -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error("SQLite prepare failed");
        }
    }

    ~SQLiteSink() override {
        if (stmt_) sqlite3_finalize(stmt_);
        if (db_) sqlite3_close(db_);
    }

    void consume(const Event& ev) override {
        std::lock_guard<std::mutex> lock(mu_);
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ev.tp.time_since_epoch()).count();
        sqlite3_reset(stmt_);
        sqlite3_bind_int64(stmt_, 1, static_cast<sqlite3_int64>(ns));
        sqlite3_bind_text(stmt_, 2, ev.source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_, 3, ev.key.c_str(), -1, SQLITE_TRANSIENT);
        auto payload = ev.payload.dump();
        sqlite3_bind_text(stmt_, 4, payload.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt_) != SQLITE_DONE) {
            spdlog::warn("SQLite insert failed: {}", sqlite3_errmsg(db_));
        }
    }

    std::string name() const override { return "sqlite"; }

private:
    sqlite3* db_ = nullptr;
    sqlite3_stmt* stmt_ = nullptr;
    std::mutex mu_;
};

std::shared_ptr<Sink> make_sqlite_sink(const std::string& path) {
    return std::make_shared<SQLiteSink>(path);
}

} // namespace crossbring

#endif // USE_SQLITE

