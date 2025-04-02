package udp

import (
	"errors"
	"fmt"
	"io"
	"net"
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

// blocking function that formats Facility Updates on each incoming callback
func (client *UdpClient) WatchMessage(message utils.UnMarshalledRequestMessage, watchInterval int, ackTimeout int, retransmission bool, maxRetries int) error {
	conn, err := net.Dial("udp4", client.HostAddr)
	if err != nil {
		return err
	}
	defer conn.Close()
	localIp := conn.LocalAddr()
	localAddr, port, err := net.SplitHostPort(localIp.String())
	if err != nil {
		return err
	}
	udpPort, err := strconv.Atoi(port)
	sockPort := udpPort + 1
	fmt.Printf("sock-port %v\n", sockPort)
	message.Port = uint16(sockPort)
	if err != nil {
		return err
	}
	conn.SetDeadline(time.Now().Add(time.Duration(ackTimeout) * time.Second))
	data, err := utils.Marshal(&message)
	if err != nil {
		return err
	}

	if retransmission {
		ackBuf := make([]byte, 3)
		count := 0
		for count < maxRetries {
			_, err := conn.Write(data)
			if err != nil {
				return err
			}
			_, err = conn.Read(ackBuf)
			if err != nil {
				return err
			}
			if string(ackBuf) == "ACK" {
				break
			}
			count++
		}
		if count == maxRetries {
			return errors.New("reached max retries")
		}

	}

	buf := make([]byte, 1024)
	n, err := conn.Read(buf)
	if err != nil {
		return err
	}
	reply, err := utils.UnMarshal(buf[:n])
	if err != nil {
		return err
	}
	utils.FormatReplyMessage(reply)

	tcpAddr := net.TCPAddr{
		IP:   net.ParseIP(localAddr),
		Port: sockPort,
	}
	fmt.Printf("tcp port at %v\n", sockPort)
	tcpListener, err := net.ListenTCP("tcp4", &tcpAddr)
	if err != nil {
		return err
	}
	defer tcpListener.Close()
	timer := time.NewTimer(time.Minute * time.Duration(watchInterval))
	connChannel := make(chan net.Conn)
	errChannel := make(chan error)
	done := make(chan struct{}, 1)
	defer close(connChannel)
	defer close(errChannel)
	go func() {
		for {
			select {
			case <-done:
				return
			default:
				conn, err = tcpListener.Accept()
				if err != nil {
					if !errors.Is(err, net.ErrClosed) {
						errChannel <- err
					}
					return
				}
				connChannel <- conn
			}
		}
	}()
	for {
		select {
		case <-timer.C:
			fmt.Println("timer has completed")
			done <- struct{}{}
			tcpListener.Close()
			return nil
		case newConn := <-connChannel:
			go func(conn net.Conn) {
				defer conn.Close()
				connBuf := make([]byte, 1024)
				for {
					fmt.Println("reading incoming messages...")
					n, err := conn.Read(connBuf)
					if errors.Is(err, io.EOF) {
						break
					}
					if err != nil {
						fmt.Printf("Error reading from connection: %v\n", err)
						break
					}
					newReply, err := utils.UnMarshal(connBuf[:n])
					if err != nil {
						fmt.Printf("Error unmarshalling message: %v\n", err)
						break
					}
					utils.FormatReplyMessage(newReply)
				}
			}(newConn)
		case err := <-errChannel:
			return err
		}
	}
}

func (client *UdpClient) SendMessage(message utils.UnMarshalledRequestMessage, timeout int, retransmission bool, maxRetries int) (*utils.UnMarshalledReplyMessage, error) {
	conn, err := net.Dial("udp4", client.HostAddr)
	if err != nil {
		return nil, err
	}
	defer conn.Close()
	data, err := utils.Marshal(&message)
	if err != nil {
		return nil, err
	}
	buf := make([]byte, 1024)
	ackBuf := make([]byte, 3)
	for attempt := 0; attempt <= maxRetries; attempt++ {
		conn.SetDeadline(time.Now().Add(time.Duration(timeout) * time.Second))
		_, err = conn.Write(data)
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				fmt.Println("Write timeout, retrying...")
				continue
			}
			return nil, fmt.Errorf("write error: %w", err)
		}

		if retransmission {
			conn.SetReadDeadline(time.Now().Add(time.Duration(timeout/3) * time.Second))
			_, err = conn.Read(ackBuf)
			if err != nil {
				if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
					fmt.Println("ACK not received, retrying...")
					continue
				}
				return nil, fmt.Errorf("ACK read error: %w", err)
			}
			if string(ackBuf) != "ACK" {
				fmt.Println("Request dropped. ACK not received")
				continue
			}
			fmt.Println("Received ACK")
		}
		conn.SetReadDeadline(time.Now().Add(time.Duration(timeout) * time.Second))

		// Reading the actual response
		n, err := conn.Read(buf)
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				fmt.Println("Reply not received from Server. Retrying...")
				continue
			}
			return nil, fmt.Errorf("response read error: %w", err)
		}

		if n > 0 {
			newReply, err := utils.UnMarshal(buf[:n])
			if err != nil {
				return nil, err
			}
			return newReply, nil
		}

	}
	return nil, errors.New("max retries reached")

}
