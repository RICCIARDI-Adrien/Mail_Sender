/** @file Main.c
 * Send a mail with a payload using libCURL.
 * @author Adrien RICCIARDI
 */
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>

//-------------------------------------------------------------------------------------------------
// Entry point
//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	int Return_Value = EXIT_FAILURE;
	CURL *Pointer_Easy_Handle;
	
	// Retrieve command-line parameters TODO : [-s sender_email_address] [-p sender_email_password] [-r recipient_email_address] [payload_file]
	
	// Initialize cURL library
	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
	{
		printf("Error : failed to initialize cURL library.\n");
		return EXIT_FAILURE;
	}
	
	// Get a handle to use for transfers
	Pointer_Easy_Handle = curl_easy_init();
	if (Pointer_Easy_Handle == NULL)
	{
		printf("Error : failed to obtain a cURL handle.\n");
		goto Exit_Uninitialize_Curl;
	}
	
	// TODO send mail
	
	Return_Value = EXIT_SUCCESS;
	
Exit_Uninitialize_Curl:
	curl_global_cleanup();
	return Return_Value;
}
