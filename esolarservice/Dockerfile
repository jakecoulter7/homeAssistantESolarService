ARG BUILD_FROM
FROM ${BUILD_FROM}

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    git

WORKDIR /app

RUN git clone https://github.com/jakecoulter7/homeAssistantESolarService.git src

WORKDIR /app/src

RUN make

CMD ["./esolard"]
