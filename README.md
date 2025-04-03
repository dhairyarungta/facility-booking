## Prerequisites
Golang (latest version)
GCC 14
`sudo apt-get install libboost-devel`
`sudo apt-get install libfmt-dev`

## Build and run server
```
  cd server
  make
./server.out [options]
```

## Run client
```
  cd client
  go run main.go
```

If the server is remote, after running the client the first command to be run is `setserver <serverIP:serverPort>` to connect the client to the remote server.

