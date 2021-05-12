# bomber
Course project for LU LSP 2021 by Valters Mednis & Liāna Stuberovska

## Compiling
Can be compiled with make, requires raylib. It can be found in many disto repositories
alternatively you have to compile it yourself: https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux

Other than that everything else is standard C.

## Running
Server can be launched on it's own. It will open on port ```3001```. To launch
the client you have to specify ip, port and desired player name as arguments.

Example:
```
./bombsrv
./bombcli 127.0.0.1 3001 Player
./bombcli 127.0.0.1 3001 Client
```

## Architecture
Client and server are pretty indapendet although they share the same code for
packet decoding and encoding.