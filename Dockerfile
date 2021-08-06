FROM docker.io/gcc:9.4.0 as build

WORKDIR /build

COPY . .
RUN make por CUSTOMLDFLAGS="-static"

FROM scratch

WORKDIR /app

COPY --from=build /build/por /app/por

EXPOSE 10000

ENTRYPOINT ["/app/por"]
