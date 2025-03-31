package test

import (
	"fmt"
	"os"
	"sync"
	"testing"
	"time"

	"github.com/davecgh/go-spew/spew"
	"github.com/dhairyarungta/facility-booking/client/pkg/udp"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/stretchr/testify/assert"
)

const (
    IP = "localhost:3000"
    TCPIP = "localhost:3001"
)
func TestHourMinutesToString(t *testing.T){
    assert := assert.New(t)
    hm := utils.HourMinutes{
        '2',
        '3',
        '5',
        '9',
    }
    spew.Dump(hm)

    time := "23:59"
    assert.Equal(time,hm.ToString())

    fmt.Printf("Converted time: %v\n",hm.ToString())

}

func TestCode101Query(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 101,
        FacilityName: "Fitness Center",
        Days: []utils.Day{
            '0','1','2',
        },
    }

    resp, err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }
    spew.Dump(resp)
    utils.FormatReplyMessage(resp)
    assert.Equal(resp.Op,uint32(101))

}

func TestCode102Create(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 102,
        FacilityName: "Fitness Center",
        Days: []utils.Day{
            '1',
        },
        StartTime: utils.HourMinutes{
            '0','9','0','0',
        },
        EndTime:utils.HourMinutes{
            '1','1','0','0',
        },
    }

    resp, err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }
    
    utils.FormatReplyMessage(resp)
    assert.Equal(resp.Op,uint32(102))
}

func TestCode103UpdateStartTime(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)
    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 103,
        Uid: 101300,
        Offset: 30,
    }

    resp, err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    utils.FormatReplyMessage(resp)
    assert.Equal(resp.Op,uint32(103))
}

func TestCode104Monitor(t *testing.T){
    // assert := assert.New(t)
    var wg sync.WaitGroup
    watchClient := udp.NewUdpClient(IP)
    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 2,
        Op: 104,
        FacilityName: "Fitness Center",
        Offset: 5,
    }

    wg.Add(1)
    go func(){
        defer wg.Done()
        err := watchClient.WatchMessage(newReq,5,5,true,5)
        if err!=nil{
            fmt.Println(err)
            return
        }
   }()

   time.Sleep(5*time.Second)
   client := udp.NewUdpClient(IP)
   bookReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 102,
        FacilityName: "Fitness Center",
        Days: []utils.Day{
            '0',
        },
        StartTime: utils.HourMinutes{
            '0','9','0','0',
        },
        EndTime:utils.HourMinutes{
            '1','1','0','0',
        },
    }

    resp,err := client.SendMessage(bookReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    utils.FormatReplyMessage(resp)
    wg.Wait()
    
    
} 

func TestCode105Capacity(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)
    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 2,
        Op: 105,
        FacilityName: "Fitness Center",
    }

    resp, err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(resp.Capacity,uint32(50))

    newReq = utils.UnMarshalledRequestMessage{
        ReqId: 3,
        Op: 105,
        FacilityName: "Swimming Pool",
    }

    resp, err = client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(resp.Capacity,uint32(30))

    newReq = utils.UnMarshalledRequestMessage{
        ReqId: 3,
        Op: 105,
        FacilityName: "Computer Lab",
    }

    resp, err = client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(resp.Capacity,uint32(40))


}

func TestCode106UpdateWidth(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)
    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 106,
        Uid: 101300,
        Offset: 30,
    }

    resp,err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(resp.Op,uint32(106))


}

func TestCode107GetFacilities(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)
    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 107,
    }

    resp,err := client.SendMessage(newReq,5,true,5)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(resp.Op,uint32(107))
    utils.FormatReplyMessage(resp)
}