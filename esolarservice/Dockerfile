FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make

WORKDIR /app

COPY . .

RUN make

RUN mkdir -p /etc

RUN printf '%s\n' \
'inverter_host = 192.168.68.106' \
'inverter_port = 1502' \
'poll_seconds = 15' \
'http_port = 80' \
'latitude = 54.672936' \
'longitude = -5.942388' \
> /etc/esolard.conf

CMD ["./esolard"]
