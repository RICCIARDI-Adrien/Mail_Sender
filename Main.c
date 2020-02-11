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
	curl_mime *Pointer_Message = NULL;
	curl_mimepart *Pointer_Message_Part;
	struct curl_slist *Pointer_Header_Strings_List = NULL, *Pointer_Recipients_Strings_List = NULL;
	CURLcode Result;
	
	// Retrieve command-line parameters TODO : -s sender_email_address -r recipient_email_address [-p sender_email_password] [-a attachment_file] message_text
	
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
		goto Exit;
	}
	
	// Create message text part
	Pointer_Message = curl_mime_init(Pointer_Easy_Handle);
	if (Pointer_Message == NULL)
	{
		printf("Error : failed to initialize cURL message.\n");
		goto Exit;
	}
	Pointer_Message_Part = curl_mime_addpart(Pointer_Message); // This pointer should be freed when whole message is freed
	if (Pointer_Message_Part == NULL)
	{
		printf("Error : failed to add text part to cURL message.\n");
		goto Exit;
	}
	if (curl_mime_data(Pointer_Message_Part, "Test message", CURL_ZERO_TERMINATED) != CURLE_OK)
	{
		printf("Error : failed to set cURL message text content.\n");
		goto Exit;
	}
	if (curl_mime_type(Pointer_Message_Part, "text/plain") != CURLE_OK)
	{
		printf("Error : failed to set cURL message text content type.\n");
		goto Exit;
	}
	
	// Create email header
	Pointer_Header_Strings_List = curl_slist_append(NULL, "From: CHANGEME"); // Create a new list by providing "NULL" as first argument
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'From' header field.\n");
		goto Exit;
	}
	Pointer_Header_Strings_List = curl_slist_append(Pointer_Header_Strings_List, "To: CHANGEME");
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'To' header field.\n");
		goto Exit;
	}
	Pointer_Header_Strings_List = curl_slist_append(Pointer_Header_Strings_List, "Subject: test");
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'Subject' header field.\n");
		goto Exit;
	}
	
	// Configure email settings
	if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_HTTPHEADER, Pointer_Header_Strings_List) != CURLE_OK)
	{
		printf("Error : failed to set CURLOPT_HTTPHEADER option.\n");
		goto Exit;
	}
	if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_MIMEPOST, Pointer_Message) != CURLE_OK)
	{
		printf("Error : failed to set CURLOPT_MIMEPOST option.\n");
		goto Exit;
	}
	if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_URL, "smtp://CHANGEME) != CURLE_OK)
	{
		printf("Error : failed to set SMTP server URL.\n");
		goto Exit;
	}
	// Authentication support
	if (curl_easy_setopt(curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_USERNAME, "CHANGEME") != CURLE_OK)
	{
		printf("Error : failed to set TLS authentication user name.\n");
		goto Exit;
	}
    if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_PASSWORD, "CHANGEME") != CURLE_OK)
	{
		printf("Error : failed to set TLS authentication password.\n");
		goto Exit;
	}
	if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_USE_SSL, CURLUSESSL_ALL) != CURLE_OK)
	{
		printf("Error : failed to enable TLS mode.\n");
		goto Exit;
	}
	
	//curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_VERBOSE, 1);
	
	// Specify all email recipients
	Pointer_Recipients_Strings_List = curl_slist_append(NULL, "CHANGEME");
	curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_MAIL_RCPT, Pointer_Recipients_Strings_List);
	
	// Send email
	Result = curl_easy_perform(Pointer_Easy_Handle);
	if (Result != CURLE_OK)
	{
		printf("Error : failed to send email (%s).\n", curl_easy_strerror(Result));
		goto Exit;
	}
	
	printf("Email was successfully send.\n");
	Return_Value = EXIT_SUCCESS;
	
Exit:
	if (Pointer_Recipients_Strings_List != NULL) curl_slist_free_all(Pointer_Recipients_Strings_List);
	if (Pointer_Header_Strings_List != NULL) curl_slist_free_all(Pointer_Header_Strings_List);
	if (Pointer_Message != NULL) curl_mime_free(Pointer_Message);
	curl_global_cleanup();
	return Return_Value;
}
