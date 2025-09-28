/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Alex Schroeder
	UIN: 134001223
	Date: 9-26-2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>  // waitpid
#include <unistd.h>    // execvp

using namespace std;

int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int m = 256; // buffer capacity variable
	bool new_channel = false; // new channel flag
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm': // handle buffer capacity
				m = atoi (optarg);
				break;
			case 'c': // handle new channel flag
				new_channel = true;
				break;
		}
	}

	pid_t server_pid = fork();
	
	if (server_pid == 0) { // child process: run the server
		char* server_args[4];
		if (server_pid == 0) {
    		execl("./server", "./server", (char*)NULL);
			execl("./server.exe", "./server.exe", (char*)NULL);
    		perror("execl failed");
    		exit(1);
		}
		
		execvp(server_args[0], server_args);
		perror("execv failed");
		exit(1);
	} else if (server_pid > 0) { // parent process: give server time to start
		sleep(3);

		FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);

		FIFORequestChannel* active_chan = chan;   // default: use control channel
		FIFORequestChannel* new_chan = nullptr;   // for cleanup later

		if (new_channel) {
    		cout << "Requesting new channel..." << endl;

    		MESSAGE_TYPE nc_msg = NEWCHANNEL_MSG;
    		chan->cwrite(&nc_msg, sizeof(MESSAGE_TYPE));

    		char new_channel_name[100];
    		chan->cread(new_channel_name, sizeof(new_channel_name));
    
    		cout << "Server created new channel: " << new_channel_name << endl;

    		new_chan = new FIFORequestChannel(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
    		active_chan = new_chan;
    		cout << "Using new channel for data transfer." << endl;
		}


		if (!filename.empty()) {
			cout << "Requesting file: " << filename << endl;
			
			filemsg size_request(0, 0); 		// get file size by sending offset=0, length=0
			int request_len = sizeof(filemsg) + filename.size() + 1;
			char* size_buf = new char[request_len];
			memcpy(size_buf, &size_request, sizeof(filemsg));
			strcpy(size_buf + sizeof(filemsg), filename.c_str());
			
			active_chan->cwrite(size_buf, request_len);
			
			__int64_t file_size;
			active_chan->cread(&file_size, sizeof(__int64_t));
			cout << "File size: " << file_size << " bytes" << endl;
			delete[] size_buf;

			system("mkdir -p received");

			string output_path = "received/" + filename;
			FILE* output_file = fopen(output_path.c_str(), "wb");
			if (!output_file) {
				perror("Failed to create output file");
				delete chan;
				return 1;
			}

			__int64_t bytes_transferred = 0;
			char* file_buffer = new char[m]; // use buffer capacity from -m flag

			int chunk_request_len = sizeof(filemsg) + filename.size() + 1;
			char* chunk_buf = new char[chunk_request_len];

			while (bytes_transferred < file_size) {
    			int chunk_size = min((__int64_t)m, file_size - bytes_transferred);

    			filemsg chunk_request(bytes_transferred, chunk_size);
    			memcpy(chunk_buf, &chunk_request, sizeof(filemsg));
    			strcpy(chunk_buf + sizeof(filemsg), filename.c_str());
    			active_chan->cwrite(chunk_buf, chunk_request_len);
    			active_chan->cread(file_buffer, chunk_size);

    			fwrite(file_buffer, 1, chunk_size, output_file);

    			bytes_transferred += chunk_size;
    			if (bytes_transferred % (1 << 20) == 0 || bytes_transferred == file_size) {
        			cout << "Transferred " << bytes_transferred << "/" << file_size << " bytes" << endl;
    			}
			}

			delete[] chunk_buf;

			fclose(output_file);
			delete[] file_buffer;
			cout << "File transfer complete: " << output_path << endl;
			
		} else if (p != 1 || t != 0.0 || e != 1) {
			char buf[256];
			datamsg data_request(p, t, e);
			
			memcpy(buf, &data_request, sizeof(datamsg));
			active_chan->cwrite(buf, sizeof(datamsg));
			
			double reply;
			active_chan->cread(&reply, sizeof(double));
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
			
		} else { 	// default behavior when no specific arguments given
			cout << "Collecting first 1000 data points for person " << p << endl;

			system("mkdir -p received");
			FILE* csv_file = fopen("received/x1.csv", "w");
			if (!csv_file) {
				perror("Failed to create received/x1.csv");
				delete chan;
				return 1;
			}
			
			char buf[256];

			for (int i = 0; i < 1000; i++) {
				double time_point = i * 0.004; // 4ms intervals
				
				// request ecg 1
				datamsg ecg1_request(p, time_point, 1);
				memcpy(buf, &ecg1_request, sizeof(datamsg));
				active_chan->cwrite(buf, sizeof(datamsg));
				
				double ecg1_value;
				active_chan->cread(&ecg1_value, sizeof(double));
				
				// request ecg 2
				datamsg ecg2_request(p, time_point, 2);
				memcpy(buf, &ecg2_request, sizeof(datamsg));
				active_chan->cwrite(buf, sizeof(datamsg));
				
				double ecg2_value;
				active_chan->cread(&ecg2_value, sizeof(double));
				
				// match format when writing
				fprintf(csv_file, "%g,%g,%g\n", time_point, ecg1_value, ecg2_value);
				
				if ((i + 1) % 100 == 0) {
					cout << "Collected " << (i + 1) << " data points..." << endl;
				}
			}
			
			fclose(csv_file);
			cout << "Data collection complete: received/x1.csv" << endl;
		}
		
		// closing the channel    
		MESSAGE_TYPE quit_msg = QUIT_MSG;
		if (new_chan) {
    		new_chan->cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
    		delete new_chan;
		}
		chan->cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
		delete chan;

		int status;
		waitpid(server_pid, &status, 0);
		
	} else {
		perror("fork failed");
		return 1;
	}
	return 0;
}