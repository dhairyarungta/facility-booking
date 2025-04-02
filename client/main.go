package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/dhairyarungta/facility-booking/client/pkg/udp"
	"github.com/dhairyarungta/facility-booking/client/pkg/utils"
	"github.com/google/uuid"
)

const (
	DefaultHostAddr      = "localhost:3000"
	DefaultTimeout       = 60
	DefaultRetransmit    = true
	DefaultMaxRetries    = 5
	DefaultWatchInterval = 10
	DefaultAckTimeout    = 5
)

func printHelp() {
	fmt.Println("Facility Booking Client")
	fmt.Println("=======================")
	fmt.Println("Commands:")
	fmt.Println("  query <facility_name> [day1 day2...]")
	fmt.Println("      - Query facility availability. Days are 0-6 (0=Monday)")
	fmt.Println("      - If no days provided, queries all days")
	fmt.Println()
	fmt.Println("  book <facility_name> <day> <start_time> <end_time>")
	fmt.Println("      - Book a facility")
	fmt.Println("      - day: 0-6 (0=Monday)")
	fmt.Println("      - time format: HHMM (e.g., 0900 for 9:00 AM)")
	fmt.Println()
	fmt.Println("  modify <booking_id> <offset_minutes>")
	fmt.Println("      - Change booking time by offset (can be negative)")
	fmt.Println()
	fmt.Println("  extend <booking_id> <additional_minutes>")
	fmt.Println("      - Extend booking duration")
	fmt.Println()
	fmt.Println("  watch <facility_name> <interval_minutes> [watch_duration_minutes]")
	fmt.Println("      - Monitor facility for updates")
	fmt.Println("      - interval_minutes: how often to receive updates")
	fmt.Println("      - watch_duration_minutes: how long to watch (default: 10)")
	fmt.Println()
	fmt.Println("  capacity <facility_name>")
	fmt.Println("      - Query facility capacity")
	fmt.Println()
	fmt.Println("  list")
	fmt.Println("      - List all available facilities")
	fmt.Println()
	fmt.Println("  help - Show this help menu")
	fmt.Println("  quit - Exit program")
}

func main() {
	client := udp.NewUdpClient(DefaultHostAddr)
	scanner := bufio.NewScanner(os.Stdin)
	uuidRand, err := uuid.NewRandom()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error generating UUID: %v\n", err)
		os.Exit(1)
	}
	reqId := uint32(uuidRand.ID())

	fmt.Println("Facility Booking Client")
	fmt.Println("Type 'help' for available commands")
	fmt.Println()

	for {
		fmt.Print("> ")
		if !scanner.Scan() {
			break
		}

		input := scanner.Text()
		if input == "" {
			continue
		}

		args := parseCommandLine(input)
		if len(args) == 0 {
			continue
		}

		cmd := strings.ToLower(args[0])

		switch cmd {
		case "help":
			printHelp()

		case "quit", "exit":
			fmt.Println("Exiting...")
			return

		case "query":
			if len(args) < 2 {
				fmt.Println("Usage: query <facility_name> [day1 day2...]")
				continue
			}

			facilityName := args[1]
			msg := utils.UnMarshalledRequestMessage{
				ReqId:        reqId,
				Op:           101,
				FacilityName: facilityName,
				Days:         []utils.Day{},
			}

			validDays := true
			if len(args) > 2 {
				for i := 2; i < len(args); i++ {
					day, err := strconv.Atoi(args[i])
					if err != nil || day < 0 || day > 6 {
						fmt.Printf("Error: Invalid day '%s' - must be between 0-6\n", args[i])
						validDays = false
						break
					}
					msg.Days = append(msg.Days, utils.Day(day+'0'))
				}

				if !validDays {
					continue
				}
			} else {
				for i := 0; i < 7; i++ {
					msg.Days = append(msg.Days, utils.Day(i+'0'))
				}
			}

			fmt.Printf("Querying availability for '%s'...\n", facilityName)
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)
			reqId++

		case "book":
			if len(args) < 5 {
				fmt.Println("Usage: book <facility_name> <day> <start_time> <end_time>")
				fmt.Println("  day: 0-6 (0=Monday)")
				fmt.Println("  time format: HHMM (e.g., 0900 for 9:00 AM)")
				continue
			}

			facilityName := args[1]
			day, err := strconv.Atoi(args[2])
			if err != nil || day < 0 || day > 6 {
				fmt.Printf("Error: Invalid day '%s' - must be between 0-6\n", args[2])
				continue
			}

			startTimeStr := args[3]
			endTimeStr := args[4]

			if len(startTimeStr) != 4 || len(endTimeStr) != 4 {
				fmt.Println("Error: Time must be in HHMM format")
				continue
			}

			var startTime, endTime utils.HourMinutes
			for i := 0; i < 4; i++ {
				if i < len(startTimeStr) {
					startTime[i] = startTimeStr[i]
				}
				if i < len(endTimeStr) {
					endTime[i] = endTimeStr[i]
				}
			}

			msg := utils.UnMarshalledRequestMessage{
				ReqId:        reqId,
				Op:           102,
				FacilityName: facilityName,
				Days:         []utils.Day{utils.Day(day + '0')},
				StartTime:    startTime,
				EndTime:      endTime,
			}

			fmt.Printf("Booking %s on day %d from %s to %s...\n",
				facilityName, day, startTime.ToString(), endTime.ToString())
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)
			if reply.Op == 100 {
				fmt.Printf("Booking ID for future reference: %d\n", reply.Uid)
			}
			reqId++

		case "modify":
			if len(args) < 3 {
				fmt.Println("Usage: modify <booking_id> <offset_minutes>")
				continue
			}

			bookingId, err := strconv.ParseUint(args[1], 10, 32)
			if err != nil {
				fmt.Println("Error: Invalid booking ID")
				continue
			}

			offset, err := strconv.ParseInt(args[2], 10, 32)
			if err != nil {
				fmt.Println("Error: Invalid offset")
				continue
			}

			msg := utils.UnMarshalledRequestMessage{
				ReqId:  reqId,
				Op:     103,
				Uid:    uint32(bookingId),
				Offset: int32(offset),
			}

			fmt.Printf("Modifying booking %d by %d minutes...\n", bookingId, offset)
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)
			reqId++

		case "extend":
			if len(args) < 3 {
				fmt.Println("Usage: extend <booking_id> <additional_minutes>")
				continue
			}

			bookingId, err := strconv.ParseUint(args[1], 10, 32)
			if err != nil {
				fmt.Println("Error: Invalid booking ID")
				continue
			}

			additionalMinutes, err := strconv.ParseInt(args[2], 10, 32)
			if err != nil || additionalMinutes <= 0 {
				fmt.Println("Error: Invalid duration (must be positive)")
				continue
			}

			msg := utils.UnMarshalledRequestMessage{
				ReqId:  reqId,
				Op:     106,
				Uid:    uint32(bookingId),
				Offset: int32(additionalMinutes),
			}

			fmt.Printf("Extending booking %d by %d minutes...\n", bookingId, additionalMinutes)
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)

			reqId++

		case "watch":
			if len(args) < 2 {
				fmt.Println("Usage: watch <facility_name> [watch_duration_minutes]")
				continue
			}

			facilityName := args[1]

			watchDuration := DefaultWatchInterval
			if len(args) > 2 {
				duration, err := strconv.Atoi(args[2])
				if err == nil && duration > 0 {
					watchDuration = duration
				}
			}

			msg := utils.UnMarshalledRequestMessage{
				ReqId:        reqId,
				Op:           104,
				FacilityName: facilityName,
				Offset:       int32(watchDuration),
			}

			fmt.Printf("Monitoring %s for %d minutes..\n",
				facilityName, watchDuration)
			err := client.WatchMessage(msg, watchDuration, DefaultAckTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
			}
			reqId++

		case "capacity":
			if len(args) < 2 {
				fmt.Println("Usage: capacity <facility_name>")
				continue
			}

			facilityName := args[1]
			msg := utils.UnMarshalledRequestMessage{
				ReqId:        reqId,
				Op:           105,
				FacilityName: facilityName,
			}

			fmt.Printf("Querying capacity for %s...\n", facilityName)
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)
			reqId++

		case "list", "facilities":
			msg := utils.UnMarshalledRequestMessage{
				ReqId: reqId,
				Op:    107,
			}

			fmt.Println("Listing all facilities...")
			reply, err := client.SendMessage(msg, DefaultTimeout, DefaultRetransmit, DefaultMaxRetries)
			if err != nil {
				fmt.Printf("Error: %v\n", err)
				continue
			}

			utils.FormatReplyMessage(reply)
			reqId++

		default:
			fmt.Println("Unknown command. Type 'help' for available commands.")
		}

		fmt.Println()
	}

	if err := scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "Error reading input: %v\n", err)
	}
}

func parseCommandLine(input string) []string {
	var args []string
	var currentArg strings.Builder
	inQuotes := false

	for _, r := range input {
		switch r {
		case '"':
			inQuotes = !inQuotes
		case ' ', '\t':
			if inQuotes {
				currentArg.WriteRune(r)
			} else if currentArg.Len() > 0 {
				args = append(args, currentArg.String())
				currentArg.Reset()
			}
		default:
			currentArg.WriteRune(r)
		}
	}

	if currentArg.Len() > 0 {
		args = append(args, currentArg.String())
	}

	for i, arg := range args {
		if strings.HasPrefix(arg, "\"") && strings.HasSuffix(arg, "\"") && len(arg) >= 2 {
			args[i] = arg[1 : len(arg)-1]
		}
	}

	return args
}
