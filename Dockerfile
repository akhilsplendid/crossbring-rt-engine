FROM ubuntu:22.04 AS build
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make cmake git ca-certificates pkg-config libzmq3-dev \
    nlohmann-json3-dev libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_HTTP_SERVER=ON -DENABLE_ZEROMQ=ON \
 && cmake --build build --config Release -j

FROM ubuntu:22.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install -y --no-install-recommends libzmq5 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/build/rt_engine /app/rt_engine
COPY configs /app/configs
EXPOSE 9100
ENTRYPOINT ["/app/rt_engine","/app/configs/config.example.json"]
