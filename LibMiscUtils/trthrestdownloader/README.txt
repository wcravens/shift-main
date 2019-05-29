Description:

(1) Related files:
    [1] cred.json : customizable and provides RESTAPI the current user's credentials; it is encrypted by a user provided key.
    [2] extractRaw.json : provides the necessary parameters to the RESTAPI so as to download specific data. I recommend only changing "ContentFieldNames" and "InstrumentIdentifiers" fields for customizing data fields (i.e. columns headers in CSV) and RICs, respectively.
    [3] main.cpp : the main downloader program. To compile it manually, please run the g++ command specified at the top of this file. To run the program, just type "./<the app name>" in the command line at the same path.

(2) Behavior:
    This program first reads the cred.json to get the user credentials that are required by TRTH REST API. Then it tries to request the Token from TRTH ("Requesting Token...").
    After the token is retrieved successfully, the program sends the customized extractRaw.json for remote-extracting the specified data ("Requesting Extraction...").
    If the data is successfully extracted, the program continues to retrieve/download the extracted data, via an istream, from the remote (using the returned JobID from the last step) ("Requesting Results...").
    After the download is done, the program unzip that GZ file into a CSV file ("Unzipping...").
