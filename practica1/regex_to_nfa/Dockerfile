FROM alpine:latest

RUN apk add --no-cache gcc musl-dev make cmake

WORKDIR /app

COPY . .

RUN chmod +x /app/validator

RUN mkdir build
WORKDIR /app/build
RUN cmake .. && make

CMD ["../validator"]
