FROM ubuntu:22.04 AS build
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_HTTP_SERVER=ON \
 && cmake --build build --config Release -j

FROM ubuntu:22.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/build/rt_engine /app/rt_engine
COPY configs /app/configs
EXPOSE 9100
ENTRYPOINT ["/app/rt_engine","/app/configs/config.example.json"]

