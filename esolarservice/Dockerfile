ARG BUILD_FROM
FROM $BUILD_FROM

RUN apk add --no-cache \
    gcc \
    musl-dev \
    make

WORKDIR /app

COPY Makefile .
COPY esolard.c .
COPY WebPages.h .
COPY icon.h .
COPY esolard.conf .

RUN make

CMD ["/app/esolard"]
