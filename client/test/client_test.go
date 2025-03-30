package test

import (
	"testing"

	//"unsafe"

	//"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/davecgh/go-spew/spew"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/stretchr/testify/assert"
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
