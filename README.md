# TcpTunnel

TcpTunnel is a versatile tool that supports port forwarding and various other functionalities.

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
g++ -g -Wall -Wextra -o tcp_tunnel tcp_tunnel.cpp
```

### Running

To run the TcpTunnel program, use the following command:

```bash
./tcp_tunnel "192.168.x.x" "0.0.0.0" "25565"
```

Replace "192.168.x.x" with the server hostname or IP address, "0.0.0.0" with the client address, and "25565" with the desired port number.

## Contributing

To contribute:

1. Fork the repository.
2. Create a new branch (git checkout -b feature-branch).
3. Make your changes.
4. Commit your changes (git commit -m 'Add some feature').
5. Push to the branch (git push origin feature-branch).
6. Open a merge request.