FROM gcc:latest

WORKDIR /app

RUN apt-get update && apt-get install -y cmake

COPY . .

RUN mkdir build && cd build && cmake .. && make

ENTRYPOINT ["build/Compiler"]
