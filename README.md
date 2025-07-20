# C++ HTTP Server

This is a small HTTP server written in C++ as a pet project. The goal was to get hands-on experience with the language and learn the basics of network programming.

## Features

- Multithreaded request handling using a custom thread pool
- Event-driven architecture using `kqueue` for monitoring socket events
- Basic support for several HTTP routes:
  - `GET` and `POST` methods
  - File creation, reading, and writing
- Gzip compression support for responses (echo)
- Code is modular and easy to extend
- Clean separation of logic and networking

> **Note:** Keep-Alive support is currently not implemented, but may be added in future updates.

## Why

This project helped me understand:
- How low-level networking works
- How to implement concurrency
- How to build scalable architecture using non-blocking I/O

## License

This project is shared for educational purposes. Feel free to explore and adapt it to your needs.
