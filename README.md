# Crossbring Real-Time Data Processing Engine (C++17)

![CI](https://github.com/akhilsplendid/crossbring-rt-engine/actions/workflows/ci.yml/badge.svg?branch=master)
![Release](https://github.com/akhilsplendid/crossbring-rt-engine/actions/workflows/release.yml/badge.svg)
[![Latest Release](https://img.shields.io/github/v/release/akhilsplendid/crossbring-rt-engine?display_name=tag)](https://github.com/akhilsplendid/crossbring-rt-engine/releases)

High-performance C++17 data processing engine for real-time analytics with a multi-threaded design and streaming integration. Simulates sensor streams, ingests external data (Arbetsförmedlingen Platsbanken jobs), processes events with low latency, and sends to sinks (console, optional SQLite).

You can say: Built a high-performance C++ data processing engine for real-time analytics with multi-threaded design and streaming integration.

## Features
- Multi-threaded engine with bounded MPMC queue
- Low-latency ingestion and in-place processing
- Sources:
  - Synthetic sensor simulator
  - File JSON source (targets AF jobs via script)
- Sinks:
  - Console logger
  - Optional SQLite storage
- JSON config driven

Optional integrations (stubs/ready hooks):
- ZeroMQ publisher (enable in CMake when libzmq available)
- Grafana via Prometheus: expose metrics through sidecar or extend app with an HTTP metrics endpoint

## Repo Layout
- `apps/rt_engine_main.cpp` – main application
- `include/crossbring/**` – public headers (engine, sources, sinks)
- `src/**` – implementations
- `configs/config.example.json` – sample config
- `scripts/fetch_af_jobs.ps1` – fetch AF job ads to JSON

## Build (CMake)
Requirements: CMake 3.16+, a C++17 compiler. Internet access for dependencies via FetchContent (nlohmann/json, spdlog).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_HTTP_SERVER=ON
cmake --build build --config Release
```

Run:
```
./build/rt_engine configs/config.example.json
```
(Windows MSVC: `build/Release/rt_engine.exe`)

## Use the AF Jobs Source
1. Fetch data to `data/af_jobs.json`:
   - Windows PowerShell:
     ```powershell
     pwsh scripts/fetch_af_jobs.ps1 -OutFile data/af_jobs.json -MaxRecords 25
     ```
2. Ensure `configs/config.example.json` has a `file_json` source pointing at `data/af_jobs.json`.
3. Start the engine. Updates to the JSON file are detected and re-emitted.

## SQLite Sink (optional)
- Enable by passing `-DENABLE_SQLITE=ON` and having `SQLite3` dev libs available.
- Then set in config:
  ```json
  "sinks": { "sqlite": { "enabled": true, "path": "data/events.sqlite" } }
  ```

## Extending
- Processors: add `Engine::Processor` lambdas to enrich/transform payloads.
- Sinks: implement `crossbring::Sink` to forward to ZeroMQ/Kafka, files, etc.
- Sources: add HTTP/Kafka/Socket sources. For HTTPS in C++, prefer Boost.Beast or cpr.

## Notes
- This scaffold keeps external deps minimal and portable.
- ZeroMQ/Kafka are optional and can be added per environment readiness.

## Target Role Alignment
- Emphasizes C++17, concurrency, event-driven design, and real-time ingestion.
- Demonstrates streaming integration and low-latency processing.
## Optional: ZeroMQ PUB and HTTPS AF Source
- ZeroMQ PUB sink
  - Build with `-DENABLE_ZEROMQ=ON` and ensure `libzmq` is installed.
  - Enable in config:
    ```json
    "sinks": { "zmq_pub": { "enabled": true, "endpoint": "tcp://*:5556" } }
    ```
- HTTPS AF source (direct API)
  - Build with `-DENABLE_CPR=ON` (cpr is fetched and built automatically).
  - Enable in config under `sources.af_https` (see `configs/config.example.json`).

## Observability
- Prometheus scrape config example: `configs/prometheus.yml` (targets default `127.0.0.1:9100`).
- Grafana dashboard example: `configs/grafana-dashboard.json` with basic counters.
- HTTP endpoints:
  - `/metrics` (Prometheus format)
  - `/recent` (JSON array)
  - `/` (simple HTML dashboard)

## ZeroMQ → WebSocket Bridge + Web UI
- Build and run engine with ZeroMQ PUB enabled in config and CMake `-DENABLE_ZEROMQ=ON`.
- Start the bridge via Docker Compose (serves a WebSocket and a minimal web UI):
  - `docker compose up -d bridge`
  - Open `http://localhost:8081/` to see live events via WebSocket.
- Bridge envs:
  - `ZMQ_ENDPOINT` (default `tcp://host.docker.internal:5556`)
  - `PORT` (default `8081`)

## Windows Build Script
- Use `scripts/build.ps1`:
  - Examples:
    - `pwsh scripts/build.ps1` (Release, HTTP server on, SQLite on)
    - `pwsh scripts/build.ps1 -EnableZmq` (enable ZeroMQ if available)
    - `pwsh scripts/build.ps1 -EnableCpr` (enable CPR AF HTTPS source)

## Engine Docker Image
- Build locally: `docker build -t crossbring/rt-engine .`
- Run: `docker run --rm -p 9100:9100 crossbring/rt-engine`

## Full Stack with Docker Compose
- Starts the engine (with ZMQ PUB on 5556), Prometheus, Grafana, and ZMQ→WS bridge:
  - `docker compose up -d`
  - Engine metrics: `http://localhost:9100/metrics`
  - Grafana: `http://localhost:3000` (admin/admin)
  - Bridge UI: `http://localhost:8081/`

## CI
- GitHub Actions workflow builds C++ and Docker images on push/PR: `.github/workflows/ci.yml`.

## Releases and GHCR Images
- Tag a release (e.g., `v0.1.0`) to trigger:
  - Cross-platform binaries attached to the GitHub Release
  - GHCR image publish for engine and bridge with tags: `latest`, `<major>.<minor>`, and full `<version>`
- Pull images:
  - Engine: `docker pull ghcr.io/akhilsplendid/crossbring-rt-engine:latest`
  - Bridge: `docker pull ghcr.io/akhilsplendid/crossbring-rt-bridge:latest`
- Visibility note: If packages appear private, set them to public in GitHub Packages settings for this repo.
