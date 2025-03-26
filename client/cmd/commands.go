package cmd

import (
	"fmt"
	"os"

	"github.com/davecgh/go-spew/spew"
	"github.com/dhairyarungta/facility-booking/client/pkg/udp"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/spf13/cobra"
)


//This file contains the request logic for each designated service


var QueryAvailability = &cobra.Command{
	Use: "query -f facility -d days...",
	Short: "Query for available timeslots for days",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		hostArr, err := cmd.Flags().GetString("address")
		if err!=nil {
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}


		facility, err := cmd.Flags().GetString("facility")
		if err!=nil {
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}


		days, err := cmd.Flags().GetStringSlice("days")

		if err!=nil {
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}

		fmt.Printf("Querying about %v for days: %v, host: %v\n",facility,days,hostArr)

		},

}

var CreateBooking = &cobra.Command{
	Use: "book -f facility -s start -e end",
	Short: "Create a booking if slot is empty or exists",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		facility, err := cmd.Flags().GetString("facility")
		start, err := cmd.Flags().GetString("start")
		end, err :=cmd.Flags().GetString("end")

		if err!=nil {
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}

		fmt.Printf("Booking timeslot (%v) - (%v) for %v\n",start,end,facility)
	},
}


var UpdateBooking = &cobra.Command{
	Use: "update -u uid -s new start time",
	Short: "Update existing booking by specifying new start time",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		uid, err := cmd.Flags().GetString("uid")
		start, err := cmd.Flags().GetString("start")
		if err!=nil{
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}

		fmt.Printf("Updating booking-id: %v, new start time: %v \n",uid,start)
	},
}

//this command will start up a new process/block the current thread and wait for updates from the server and display on a feed similar to a watch command
var StartMonitoring = &cobra.Command{
	Use: "watch -f facility -i interval",
	Short: "Watch for updates on your desired facility",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		facility , err := cmd.Flags().GetString("facility")
		interval, err := cmd.Flags().GetInt("interval")
		if err!=nil{
			fmt.Println("Missing 1 or more params")
			os.Exit(1)
		}

		fmt.Printf("Monitoring %v for %v \n",facility,interval)

	},

}

var TestMarshalUnMarshal = &cobra.Command{
	Use: "test",
	Short: "Used to test marshal and unmarshal at server and client",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ip, err := cmd.Flags().GetString("address")
		if err!=nil{
			fmt.Println("Unable to find address")
			os.Exit(1)
		}
		fmt.Printf("Sending Test request to %v\n",ip)

		client := udp.NewUdpClient(ip)
		fmt.Println("starting client")
		//101 test
		newReq := utils.UnMarshalledRequestMessage{
			ReqId: 1,
			Uid: 1,
			Op: 101,
			Days: []utils.Day{
				'0','1','2',
			},
			FacilityName: "test",

		}
		res,err := client.SendMessage(newReq,5,true,1)
		fmt.Printf("%v\n",client.HostAddr)
		// if(res.Op != 101 || res.Uid != 1  || res.FacilityNames[0] != "test" || res.Availabilities[0].Day != 0 || res.Availabilities[0].TimeSlots[0].StartTime  != utils.Hourminute{0,0,0,0} || res.Availabilities[0].TimeSlots[0].EndTime != utils.Hourminute{1,1,5,9}){
		// 	fmt.Println("101 Invalid reply format")
		// 	os.Exit(1)

		// }
		spew.Dump(res)
		if err!= nil{
			fmt.Printf("101 failed to send request to server: %v",err)
			os.Exit(1)
		}

		// //102 test
		// newReq = utils.UnMarshalledRequestMessage{
		// 	ReqId: 1,
		// 	Uid: 1,
		// 	Op: 102,
		// 	Days: []utils.Day{
		// 		1,
		// 	},
		// 	StartTime: utils.Hourminute{
		// 		0,0,0,0,
		// 	},
		// 	EndTime: utils.Hourminute{
		// 		1,1,5,9,
		// 	},

		// }

		// res ,err = client.SendMessage(newReq,5,true,1)
		// spew.Dump(res)
		// if(res.Op != 102 || res.Uid != 1){
		// 	fmt.Println("102 invalid reply format")
		// 	os.Exit(1)
		// }
		// if err!= nil{
		// 	fmt.Println("failed to send to server")	
		// 	os.Exit(1)
		// }


		// //105 test
		// newReq = utils.UnMarshalledRequestMessage{
		// 	ReqId: 1,
		// 	Uid: 1,
		// 	Op: 105,
		// 	FacilityName: "test",
		// }

		// res,err = client.SendMessage(newReq,5,true,1)
		// spew.Dump(res)
		// if(res.Op != 105 || res.Uid != 1 || res.Capacity != 777){
		// 	fmt.Println("105 invalid reply format")
		// 	os.Exit(1)
		// }

		// if err!= nil{
		// 	fmt.Println("failed to send to server")	
		// 	os.Exit(1)
		// }

		
	},

}


func init(){
	QueryAvailability.Flags().StringP("facility","f","","desired facility")
	QueryAvailability.MarkFlagRequired("facility")
	QueryAvailability.Flags().StringSliceP("days","d",[]string{""},"desired days")
	QueryAvailability.MarkFlagRequired("days")

	CreateBooking.Flags().StringP("facility","f","","desired facility")
	CreateBooking.MarkFlagRequired("facility")
	CreateBooking.Flags().StringP("start","s","0000","start time of your booking")
	CreateBooking.MarkFlagRequired("start")
	CreateBooking.Flags().StringP("end","e","2359","end time of your booking")
	CreateBooking.MarkFlagRequired("end")

	UpdateBooking.Flags().StringP("uid","u","","booking uid")
	UpdateBooking.MarkFlagRequired("uid")
	UpdateBooking.Flags().StringP("start","s","","new start time of your booking")
	UpdateBooking.MarkFlagRequired("start")


	StartMonitoring.Flags().StringP("facility","f","","desired facility")
	StartMonitoring.MarkFlagRequired("facility")
	StartMonitoring.Flags().IntP("interval","i",0,"enter duration of monitoring in seconds")
	StartMonitoring.MarkFlagRequired("interval")


}
	



