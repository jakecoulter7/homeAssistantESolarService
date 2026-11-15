FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    git

WORKDIR /app

RUN git clone https://github.com/woeperbe/eSolarService.git .

RUN make

RUN mkdir -p /etc

RUN printf '%s\n' \
'inverter_host = 192.168.68.110' \
'inverter_port = 1502' \
'poll_seconds = 15' \
'http_port = 80' \
> /etc/esolard.conf

CMD ["./esolard"]
