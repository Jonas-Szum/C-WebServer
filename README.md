# C-WebServer
Simple web server that handles multiple client connections and CGI requests

To run, ./webServer (Port-Number) (Directory which stores important files and CGI folder)
Example: when SSH'd into systems1.cs.uic.edu, running ./webServer 9185 WWW will create a website (systems1.cs.uic.edu:9185/)
that is accessible to anyone connected to UIC wifi. A cgi request is formatted such that the name of an executable in the cgi
folder is paired with it's aruments like so: systems1.cs.uic.edu:9185/executable?arg1&arg2. Right now, the only executable
in the cgi folder is one called format_string. The webserver also handles 404 errors gracefully.
