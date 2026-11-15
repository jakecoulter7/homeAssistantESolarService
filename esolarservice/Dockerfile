FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    git

WORKDIR /app

RUN git clone https://github.com/jakecoulter7/homeAssistantESolarService.git src

WORKDIR /app/src

RUN make

COPY run.sh /run.sh
RUN chmod +x /run.sh

CMD ["/run.sh"]
