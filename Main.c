/** @file Main.c
 * Send a mail with a payload using libCURL.
 * @author Adrien RICCIARDI
 */
#include <curl/curl.h>
#include <iniparser/iniparser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//-------------------------------------------------------------------------------------------------
// Private constants
//-------------------------------------------------------------------------------------------------
/** The configuration default file name.
 * @todo Add a command-line parameter to specify another configuration file.
 */
#define MAIN_CONFIGURATION_FILE_NAME "Configuration.ini"

//-------------------------------------------------------------------------------------------------
// Private types
//-------------------------------------------------------------------------------------------------
/** All needed settings to be able to send an email. */
typedef struct
{
	char String_Sender_Email[256]; //!< The address to send the email from.
	char String_SMTP_Server[256]; //!< The SMTP server URL.
	char String_Authentication_User_Name[256]; //!< The name to provide to SSL/TLS authentication step.
	char String_Authentication_Password[256]; //!< The password to provide to SSL/TLS authentication step.
} TMainEmailConfiguration;

//-------------------------------------------------------------------------------------------------
// Private variables
//-------------------------------------------------------------------------------------------------
/** The currently in use configuration. */
static TMainEmailConfiguration Main_Email_Configuration;

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------
/** Load the requested configuration data from the configuration file.
 * @param Pointer_String_Configuration_Key_Name The configuration to load (this is the INI section identifier).
 * @return -1 if an error occurred,
 * @return 0 on success.
 */
static int MainLoadConfiguration(char *Pointer_String_Configuration_Key_Name)
{
	int Return_Value = -1;
	dictionary *Pointer_Ini_Dictionary = NULL;
	char String_Temporary[1024];
	
	// Parse configuration file
	Pointer_Ini_Dictionary = iniparser_load(MAIN_CONFIGURATION_FILE_NAME);
	if (Pointer_Ini_Dictionary == NULL)
	{
		printf("Error : could not find configuration file.\n");
		goto Exit;
	}
	
	// Check whether requested configuration is present
	if (!iniparser_find_entry(Pointer_Ini_Dictionary, Pointer_String_Configuration_Key_Name))
	{
		printf("The requested email configuration was not found in the configuration file.\n");
		goto Exit;
	}
	
	// Load sender email
	snprintf(String_Temporary, sizeof(String_Temporary), "%s:SenderEmail", Pointer_String_Configuration_Key_Name);
	strncpy(Main_Email_Configuration.String_Sender_Email, iniparser_getstring(Pointer_Ini_Dictionary, String_Temporary, ""), sizeof(Main_Email_Configuration.String_Sender_Email));
	Main_Email_Configuration.String_Sender_Email[sizeof(Main_Email_Configuration.String_Sender_Email) - 1] = 0; // Make sure string is terminated
	if (Main_Email_Configuration.String_Sender_Email[0] == 0)
	{
		printf("Error : no sender email is provided in configuration \"%s\", please add a key named \"SenderEmail\".\n", Pointer_String_Configuration_Key_Name);
		goto Exit;
	}
	
	// Load SMTP server
	snprintf(String_Temporary, sizeof(String_Temporary), "%s:SmtpServer", Pointer_String_Configuration_Key_Name);
	strncpy(Main_Email_Configuration.String_SMTP_Server, iniparser_getstring(Pointer_Ini_Dictionary, String_Temporary, ""), sizeof(Main_Email_Configuration.String_SMTP_Server));
	Main_Email_Configuration.String_SMTP_Server[sizeof(Main_Email_Configuration.String_SMTP_Server) - 1] = 0; // Make sure string is terminated
	if (Main_Email_Configuration.String_SMTP_Server[0] == 0)
	{
		printf("Error : no SMTP server is provided in configuration \"%s\", please add a key named \"SmtpServer\".\n", Pointer_String_Configuration_Key_Name);
		goto Exit;
	}
	
	// Authentication user name
	snprintf(String_Temporary, sizeof(String_Temporary), "%s:UserName", Pointer_String_Configuration_Key_Name);
	strncpy(Main_Email_Configuration.String_Authentication_User_Name, iniparser_getstring(Pointer_Ini_Dictionary, String_Temporary, ""), sizeof(Main_Email_Configuration.String_Authentication_User_Name));
	Main_Email_Configuration.String_Authentication_User_Name[sizeof(Main_Email_Configuration.String_Authentication_User_Name) - 1] = 0; // Make sure string is terminated
	if (Main_Email_Configuration.String_Authentication_User_Name[0] == 0) printf("Information : no authentication user name is provided in configuration \"%s\", disabling SSL/TLS authentication. Please add two keys named \"UserName\" and \"Password\" to enable authentication.\n", Pointer_String_Configuration_Key_Name);
	
	// Authentication password
	snprintf(String_Temporary, sizeof(String_Temporary), "%s:Password", Pointer_String_Configuration_Key_Name);
	strncpy(Main_Email_Configuration.String_Authentication_Password, iniparser_getstring(Pointer_Ini_Dictionary, String_Temporary, ""), sizeof(Main_Email_Configuration.String_Authentication_Password));
	Main_Email_Configuration.String_Authentication_Password[sizeof(Main_Email_Configuration.String_Authentication_Password) - 1] = 0; // Make sure string is terminated
	// Make sure a password is provided if a user name has been provided
	if ((Main_Email_Configuration.String_Authentication_Password[0] == 0) && (Main_Email_Configuration.String_Authentication_User_Name[0] != 0))
	{
		printf("Error : no authentication password is provided in configuration \"%s\" while an authentication user name is present. Please add a key named \"Password\" to allow SSL/TLS authentication to work or clear \"UserName\" key content to disable SSL/TLS authentication.\n", Pointer_String_Configuration_Key_Name);
		goto Exit;
	}
	
	Return_Value = 0;

Exit:
	if (Pointer_Ini_Dictionary != NULL) iniparser_freedict(Pointer_Ini_Dictionary);
	return Return_Value;
}

/** Read up to the specified amount of characters and remove any trailing newline character.
 * @param Maximum_Characters_Count Up to this amount of characters will be read.
 * @param Pointer_String_User_Input On output, contain the text the user wrote.
 */
static void MainReadUserInput(size_t Maximum_Characters_Count, char *Pointer_String_User_Input)
{
	size_t Length;
	
	// Read user input
	fgets(Pointer_String_User_Input, Maximum_Characters_Count, stdin);
	
	// Remove any trailing newline character
	Length = strlen(Pointer_String_User_Input);
	if (Pointer_String_User_Input[Length - 1] == '\n') Pointer_String_User_Input[Length - 1] = 0;
}

//-------------------------------------------------------------------------------------------------
// Entry point
//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	int Return_Value = EXIT_FAILURE, i, Is_Verbose_Mode_Enabled = 0;
	CURL *Pointer_Easy_Handle;
	curl_mime *Pointer_Message = NULL;
	curl_mimepart *Pointer_Message_Part;
	struct curl_slist *Pointer_Header_Strings_List = NULL, *Pointer_Recipients_Strings_List = NULL;
	CURLcode Result;
	char String_Temporary[1024], String_Recipient[768], String_User_Input[768];
	
	// Retrieve command-line parameters
	for (i = 1; i < argc; i++) // Start from 1 to bypass program name
	{
		// Should usage message be displayed ?
		if (strcmp("--help", argv[i]) == 0)
		{
			printf("Usage : %s [--verbose] [--help]\n", argv[0]);
			return EXIT_SUCCESS;
		}
		// Should verbose mode be enabled ?
		else if (strcmp("--verbose", argv[i]) == 0) Is_Verbose_Mode_Enabled = 1;
	}
	
	// Ask user for an email configuration
	printf("Please enter email configuration name : ");
	MainReadUserInput(sizeof(String_Temporary), String_Temporary);
	
	// Try to load requested configuration
	if (MainLoadConfiguration(String_Temporary) != 0) return EXIT_FAILURE;
	printf("Configuration \"%s\" has been successfully loaded.\n", String_Temporary);
	
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
	
	// Create email header
	snprintf(String_Temporary, sizeof(String_Temporary), "From: %s", Main_Email_Configuration.String_Sender_Email);
	Pointer_Header_Strings_List = curl_slist_append(NULL, String_Temporary); // Create a new list by providing "NULL" as first argument
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'From' header field.\n");
		goto Exit;
	}
	
	// Ask user for recipient
	printf("Please enter recipient email address : ");
	MainReadUserInput(sizeof(String_Recipient), String_Recipient);
	snprintf(String_Temporary, sizeof(String_Temporary), "To: %s", String_Recipient);
	Pointer_Header_Strings_List = curl_slist_append(Pointer_Header_Strings_List, String_Temporary);
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'To' header field.\n");
		goto Exit;
	}
	
	// Ask user for message subject
	printf("Please enter message subject : ");
	MainReadUserInput(sizeof(String_User_Input), String_User_Input);
	snprintf(String_Temporary, sizeof(String_Temporary), "Subject: %s", String_User_Input);
	Pointer_Header_Strings_List = curl_slist_append(Pointer_Header_Strings_List, String_Temporary);
	if (Pointer_Header_Strings_List == NULL)
	{
		printf("Error : failed to set 'Subject' header field.\n");
		goto Exit;
	}
	
	// Ask user for message text
	printf("Please enter message text (no more than 700 characters, no new line character) : ");
	MainReadUserInput(sizeof(String_User_Input), String_User_Input);
	
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
	if (curl_mime_data(Pointer_Message_Part, String_User_Input, CURL_ZERO_TERMINATED) != CURLE_OK)
	{
		printf("Error : failed to set cURL message text content.\n");
		goto Exit;
	}
	if (curl_mime_type(Pointer_Message_Part, "text/plain; charset=\"UTF-8\"") != CURLE_OK)
	{
		printf("Error : failed to set cURL message text content type.\n");
		goto Exit;
	}
	
	// Ask user for an optional attachment
	printf("Enter an attached file path (leave empty for no attachment) : ");
	MainReadUserInput(sizeof(String_Temporary), String_Temporary);
	if (String_Temporary[0] != 0)
	{
		Pointer_Message_Part = curl_mime_addpart(Pointer_Message);
		if (Pointer_Message_Part == NULL)
		{
			printf("Error : failed to add attachment part to cURL message.\n");
			goto Exit;
		}
		if (curl_mime_filedata(Pointer_Message_Part, String_Temporary) != CURLE_OK)
		{
			printf("Error : failed to attach file \"%s\".\n", String_Temporary);
			goto Exit;
		}
		if (curl_mime_encoder(Pointer_Message_Part, "base64") != CURLE_OK)
		{
			printf("Error : failed to encode attached file in base 64.\n");
			goto Exit;
		}
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
	snprintf(String_Temporary, sizeof(String_Temporary), "smtp://%s", Main_Email_Configuration.String_SMTP_Server);
	if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_URL, String_Temporary) != CURLE_OK)
	{
		printf("Error : failed to set SMTP server URL.\n");
		goto Exit;
	}
	
	// Enable SSL/TLS authentication if present in configuration file
	if (Main_Email_Configuration.String_Authentication_User_Name[0] != 0)
	{
		if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_USERNAME, Main_Email_Configuration.String_Authentication_User_Name) != CURLE_OK)
		{
			printf("Error : failed to set TLS authentication user name.\n");
			goto Exit;
		}
		if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_PASSWORD, Main_Email_Configuration.String_Authentication_Password) != CURLE_OK)
		{
			printf("Error : failed to set TLS authentication password.\n");
			goto Exit;
		}
		if (curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_USE_SSL, CURLUSESSL_ALL) != CURLE_OK)
		{
			printf("Error : failed to enable TLS mode.\n");
			goto Exit;
		}
	}
	
	// Enable verbose mode if requested to
	if (Is_Verbose_Mode_Enabled) curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_VERBOSE, 1);
	
	// Specify all email recipients
	Pointer_Recipients_Strings_List = curl_slist_append(NULL, String_Recipient);
	curl_easy_setopt(Pointer_Easy_Handle, CURLOPT_MAIL_RCPT, Pointer_Recipients_Strings_List);
	
	// Send email
	printf("Sending message...\n");
	Result = curl_easy_perform(Pointer_Easy_Handle);
	if (Result != CURLE_OK)
	{
		printf("Error : failed to send email (%s).\n", curl_easy_strerror(Result));
		goto Exit;
	}
	
	printf("Message was successfully send.\n");
	Return_Value = EXIT_SUCCESS;
	
Exit:
	if (Pointer_Recipients_Strings_List != NULL) curl_slist_free_all(Pointer_Recipients_Strings_List);
	if (Pointer_Header_Strings_List != NULL) curl_slist_free_all(Pointer_Header_Strings_List);
	if (Pointer_Message != NULL) curl_mime_free(Pointer_Message);
	curl_global_cleanup();
	return Return_Value;
}
