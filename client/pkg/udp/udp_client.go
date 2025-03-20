package udp

import (
	"errors"
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

func (client *UdpClient) sendMessage(message utils.NetworkMessage,timeout int, retransmission bool, maxRetries int) ([]byte,error){
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

	if(retransmission){

		ackBuf := make([]byte,3)
		count := 0
		for count < maxRetries{
			_, err = conn.Write(data)
			_,err := conn.Read(ackBuf)
			if err!=nil{
				return nil, err
			}

			if(string(ackBuf) == "ACK"){
				break
			}
			count ++
		}

		if(count == maxRetries){
			return nil, errors.New("reached max retries")
		}
	}

	buf := make([]byte,1024)
	n, err := conn.Read(buf)
	if err !=nil{
		return nil, err
	}
	return buf[:n], nil
}