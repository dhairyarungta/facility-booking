package main

import (
	"fmt"
	"os"

	"github.com/dhairyarungta/facility-booking/client/cmd"
	"github.com/spf13/cobra"
)
var HostAddr string
var rootCmd *cobra.Command

func init(){

	rootCmd = &cobra.Command{
		Use: "fba [-a addr]",
		Short: "One stop interface for booking and querying about facilities in NTU",
		Args: cobra.NoArgs,
		Run: func(cmd *cobra.Command, args []string){
			hostAddr ,err := cmd.Flags().GetString("address")
			if err!=nil{
				fmt.Println("host address not found")
				os.Exit(1)
			}
			fmt.Printf("Establishing connection with host %v\n",hostAddr)
		},
	}

	rootCmd.PersistentFlags().StringP("address","a","127.0.0.1:8080","address of facility server")
	rootCmd.AddCommand(cmd.QueryAvailability)
	rootCmd.AddCommand(cmd.CreateBooking)
	rootCmd.AddCommand(cmd.UpdateBooking)
	rootCmd.AddCommand(cmd.StartMonitoring)

}

func main(){


	if err := rootCmd.Execute(); err!=nil{
		fmt.Fprintln(os.Stderr,err)
	}
}
