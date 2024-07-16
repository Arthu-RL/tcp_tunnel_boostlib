# TcpTunnel

TCP server wrote in c++.

## Table of Contents

- [Usage](#usage)
  - [Compiling](#compiling)
  - [Running](#running)
- [Contributing](#contributing)

## Usage

### Dependencies 

- [plog](https://github.com/SergiusTheBest/plog)
- [boost](https://www.boost.org/)

### Compiling

To compile the TcpTunnel program, use the following command:

```bash
g++ -std=c++11 -Wall -Wextra -o tcp_server tcp_server.cpp -lboost_system -lplog
```

Alternatively, you can compile using build.sh script:

```bash
./build.sh 1
```

### Running

To run the TcpTunnel program, use the following command:

```bash
./tcp_server "0.0.0.0" "25565"
```

## Contributing

To contribute:

1. Fork the repository.
2. Create a new branch (git checkout -b feature-branch).
3. Make your changes.
4. Commit your changes (git commit -m 'Add some feature').
5. Push to the branch (git push origin feature-branch).
6. Open a merge request.