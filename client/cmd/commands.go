package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

/*TODO 
insert client logic in all cobra Run functions, flags will contain the arguments
*/


//This file contains the request logic for each designated service


var QueryAvailability = &cobra.Command{
	Use: "query -f facility -d days...",
	Short: "Query for available timeslots for days",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {
		hostArr, err := cmd.Flags().GetString("address")
		facility, err := cmd.Flags().GetString("facility")
		days, err :=cmd.Flags().GetStringSlice("days")

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

		fmt.Printf("Updating booking-id: %v, new start time: %v ",uid,start)
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

		fmt.Println("Monitoring %v for %v ",facility,interval)

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
	



