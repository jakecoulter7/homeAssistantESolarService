FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    git

WORKDIR /app

RUN git clone https://github.com/woeperbe/eSolarService.git .

RUN make

CMD ["./esolard"]
