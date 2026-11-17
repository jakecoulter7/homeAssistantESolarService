FROM alpine:latest

# gcc/make to build esolard, jq to read the add-on options, tzdata so "timezone" works
RUN apk add --no-cache \
    gcc \
    musl-dev \
    make \
    jq \
    tzdata

WORKDIR /app

COPY . .

RUN make

# run.sh may have Windows line endings if it was edited on Windows
RUN sed -i 's/\r$//' run.sh && chmod +x run.sh

CMD ["./run.sh"]
