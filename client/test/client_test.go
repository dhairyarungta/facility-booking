package test

import (
	"fmt"
	"os"
	"testing"
	"github.com/dhairyarungta/facility-booking/client/pkg/udp"
	"github.com/davecgh/go-spew/spew"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/stretchr/testify/assert"
)

const (
    IP string = "localhost:3000"
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

}

func TestMarshalUnMarshal101(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 101,
        Days: []utils.Day{
            '0',
        },
        FacilityName: "Student Lounge",
    }

    res, err := client.SendMessage(newReq,5,true,1)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    num := uint32(101)



    assert.Equal(num, res.Op)
    assert.Equal(utils.TimeSlot{
        StartTime: utils.HourMinutes{
            '0','0','0','0',
        },
        EndTime: utils.HourMinutes{
            '2','3','5','9',
        },
    }, res.Availabilities[0].TimeSlots[0])

    spew.Dump(res)


}

func TestMarshalUnMarshal102(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 102,
        Days: []utils.Day{
            '0',
        },
        FacilityName: "Student Lounge",
        StartTime: utils.HourMinutes{
            '1','0','0','0',
        },
        EndTime: utils.HourMinutes{
            '1','1','0','0',
        },
    }

    res,err := client.SendMessage(newReq,5,true,1)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(uint32(102), res.Op)
    spew.Dump(res)

}

func TestMarshalUnMarshal103(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 103,
        Offset: 60,
    }

    res,err := client.SendMessage(newReq,5,true,1)
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(uint32(103), res.Op)
    spew.Dump(res)

}

func TestMarshalUnMarshal104(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 104,
        Offset: 60,
        FacilityName: "Student Lounge",
    }

    res,err := client.SendMessage(newReq,5,true,1)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(uint32(104),res.Op)


}


func TestMarshalUnMarshal105(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 105,
        FacilityName: "Student Lounge",
    }

    res, err := client.SendMessage(newReq,5,true,1)
    if err != nil{
        fmt.Println(res)
        os.Exit(1)
    }

    assert.Equal(uint32(350), res.Capacity)

}

func TestMarshalUnMarshal106(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    // createReq := utils.UnMarshalledRequestMessage{
    //     ReqId: 1,
    //     Op: 102,
    //     Days: []utils.Day{
    //         2,
    //     },
    //     StartTime: utils.HourMinutes{
    //         1,0,0,0,
    //     },
    //     EndTime: utils.HourMinutes{
    //         1,1,0,0,
    //     },
    // }

    // res,err := client.SendMessage(createReq,5,true,1)
    // if err!=nil{
    //     fmt.Println(err)
    //     os.Exit(1)
    // }

    // assert.Equal(102, res.Op)

    delReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Uid: 1,
        Op: 106,
    }

    delRes, err := client.SendMessage(delReq,5,true,1)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(uint32(106), delRes.Op)
    spew.Dump(delRes)

}

func TestMarshalUnMarshal107(t *testing.T){
    assert := assert.New(t)
    client := udp.NewUdpClient(IP)

    newReq := utils.UnMarshalledRequestMessage{
        ReqId: 1,
        Op: 107,
    }

    res, err := client.SendMessage(newReq,5,true,1)
    if err!=nil{
        fmt.Println(err)
        os.Exit(1)
    }

    assert.Equal(uint32(107), res.Op)
    assert.Equal([]string{
        "Fitness Center",
        "Swimming Pool",
    },res.FacilityNames)
    spew.Dump(res)
}

