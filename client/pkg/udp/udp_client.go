package udp

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"strconv"
	"time"

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

//blocking function that formats Facility Updates on each incoming callback
func (client *UdpClient) WatchMessage( message utils.UnMarshalledRequestMessage,watchInterval int,ackTimeout int,retransmission bool,maxRetries int) error {
	conn, err := net.Dial("udp4",client.HostAddr)
	if err!=nil{
		return err
	}
	defer conn.Close()
	localIp := conn.LocalAddr()
	localAddr, port, err := net.SplitHostPort(localIp.String())
	if err!=nil{
		return err
	}
	udpPort,err := strconv.Atoi(port)
	sockPort := udpPort + 1
	fmt.Printf("sock-port %v\n",sockPort)
	message.Port = uint16(sockPort)
	if err!=nil{
		return err
	}
	conn.SetDeadline(time.Now().Add(time.Duration(ackTimeout)*time.Second))
	data, err := utils.Marshal(&message)
	if err!=nil{
		return err
	}

	if(retransmission){
		ackBuf := make([]byte,3)
		count := 0
		for count < maxRetries {
			_,err := conn.Write(data)
			if err!=nil{
				return err
			}
			_,err = conn.Read(ackBuf)
			if err !=nil{
				return err
			}
			if(string(ackBuf) == "ACK"){
				fmt.Printf("Recieved %c\n",ackBuf)
				break
			}
			count ++
		}
		if(count == maxRetries){
			return errors.New("reached max retries")
		}
		
	}

	buf := make([]byte,1024)
	n,err := conn.Read(buf)
	if err!=nil{
		return err
	}
	reply,err := utils.UnMarshal(buf[:n])
	if err!=nil{
		return err
	}
	utils.FormatReplyMessage(reply)

	tcpAddr := net.TCPAddr{
		IP: net.ParseIP(localAddr),
		Port: sockPort,
	}
	fmt.Printf("tcp port at %v\n",sockPort)
	tcpListener, err := net.ListenTCP("tcp4",&tcpAddr)
	if err!=nil{
		return err
	}
	conn, err = tcpListener.Accept()
	if err!=nil{
		return err
	}
	conn.SetDeadline(time.Now().Add(time.Duration(watchInterval)*time.Minute))
	buf = make([]byte,1024)
	for {
		fmt.Println("Reading incoming messages...")
		n, err := conn.Read(buf)
		if errors.Is(err,io.EOF){
			return errors.New("server closed connection")
		}
		if errors.Is(err,os.ErrDeadlineExceeded){
			fmt.Println("timeout exceeded")
			return nil
		}
		if err !=nil{
			return err
		}
		newReply,err := utils.UnMarshal(buf[:n])
		if err!=nil{
			return err
		}
		utils.FormatReplyMessage(newReply)
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
			if err!=nil{
				return nil,err
			}
			_,err := conn.Read(ackBuf)
			if err!=nil{
				return nil, err
			}

			if(string(ackBuf) == "ACK"){
				fmt.Printf("Recieved ACK %c\n",ackBuf)
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
	newReply,err := utils.UnMarshal(buf[:n])
	if err!=nil{
		return nil,err     
	}
	return newReply, nil
}

