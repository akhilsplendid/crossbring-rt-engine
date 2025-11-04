# Crossbring Real-Time Data Processing Engine (C++17)

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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
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

