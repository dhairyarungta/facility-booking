package udp

import (
	"errors"
	"fmt"
	"io"
	"net"
	"time"

	"github.com/davecgh/go-spew/spew"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
)

type UdpClient struct {
	HostAddr string

}

func NewUdpClient(hostAddr string) UdpClient {
	return UdpClient{
		HostAddr: hostAddr,
	}
}

func (client *UdpClient) SendMessage(message utils.UnMarshalledRequestMessage,timeout int, retransmission bool, maxRetries int) (* utils.UnMarshalledReplyMessage,error){
	conn, err := net.Dial("udp4",client.HostAddr)
	if err!=nil{
		return nil, err
	}
	conn.SetDeadline(time.Now().Add(time.Duration(timeout) * time.Second))
	defer conn.Close()
	data, err := utils.Marshal(&message)
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
				fmt.Printf("Recieved ACK %v",ackBuf)
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
	if err !=nil && !errors.Is(err,io.EOF){
		return nil, err
	}

	fmt.Println(n)
	spew.Dump(buf[:n])
	newReply,err := utils.UnMarshal(buf[:n])
	if err!=nil{
		return nil,err     
	}
	return newReply, nil
}

