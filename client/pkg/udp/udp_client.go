package udp

import (
	"errors"
	"io"
	"net"
	"time"

	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
)

type UdpClient struct {
	hostAddr string

}

func NewUdpClient(hostAddr string) UdpClient {
	return UdpClient{
		hostAddr,
	}
}

func (client *UdpClient) sendMessage(message utils.NetworkMessage,timeout int ) ([]byte,error){
	conn, err := net.Dial("udp",client.hostAddr)
	if err!=nil{
		return nil, err
	}
	conn.SetDeadline(time.Now().Add(time.Duration(timeout) * time.Second))
	defer conn.Close()
	data, err := message.Marshal()
	if err!=nil{
		return nil,err
	}

	buf := make([]byte,1024)
	_, err = conn.Write(data)

	for {
		_, err := conn.Read(buf)
		if errors.Is(err,io.EOF){
			break
		}

		if err !=nil{
			return nil, err
		}
	}

	return buf, nil
}