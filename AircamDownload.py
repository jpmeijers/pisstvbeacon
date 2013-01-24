#!/usr/bin/python
#
# Ubiquity AirOS (camera) login script in python. Downloads a snopshot from the
# camera and saves it as AircamImage.jpg in the current directory.
# Original script by xistence, source: http://www.exploit-db.com/exploits/23735/
# Modified by JP Meijers, 24-01-2013
# Original did not contain any license and therefore we assume it is public domain.
#
#+--------------------------------------------------------------------------------------------------------------------------------+
# Exploit Title     : Ubiquiti AirOS <= 5.5.2 Remote POST-Auth Root Command Execution
# Date              : 12-28-2012
# Author            : xistence (xistence<[AT]>0x90.nl)
# Software link     : http://www.ubnt.com/eula/?BACK=/downloads/XM-v5.5.2.build14175.bin
# Vendor site       : http://www.ubnt.com/
# Version           : 5.5.2 and lower
# Tested on         : PicoStation M2 (hardware)
#
# Vulnerability     : The http://<IP>/test.cgi "essid" parameter is not sanitized for input which allows for execution of operating
#                     system commands. The parameter input field can be like this to create a file /tmp/test.txt:
#                     "LINKTEST & /bin/touch /tmp/test.txt #"
#             Authentication to the web site is necessary to exploit this vulnerability.
#+--------------------------------------------------------------------------------------------------------------------------------+
 
 
import urllib, urllib2, cookielib, sys, random, mimetools, mimetypes, itertools, time

 
#Login details for the AirCam
rhost = "192.168.0.40"
webUser = "cam"
webPass = ""
 
 
 
 
class MultiPartForm(object):
    """Accumulate the data to be used when posting a form."""
 
    def __init__(self):
        self.form_fields = []
        self.files = []
        self.boundary = mimetools.choose_boundary()
        return
 
    def get_content_type(self):
        return 'multipart/form-data; boundary=%s' % self.boundary
 
    def add_field(self, name, value):
        """Add a simple field to the form data."""
        self.form_fields.append( ( name, value ) )
        return
    
    def __str__(self):
        """Return a string representing the form data, including attached files."""
        # Build a list of lists, each containing "lines" of the
        # request.  Each part is separated by a boundary string.
        # Once the list is built, return a string where each
        # line is separated by '\r\n'.
        parts = []
        part_boundary = '--' + self.boundary
 
        # Add the form fields
        parts.extend(
            [ part_boundary,
              'Content-Disposition: form-data; name="%s"' % name,
              '',
              value,
            ]
            for name, value in self.form_fields
            )
 
        # Flatten the list and add closing boundary marker,
        # then return CR+LF separated data
        flattened = list( itertools.chain( *parts) )
        flattened.append( '--' + self.boundary + '--' )
        flattened.append( '' )
        return '\r\n'.join( flattened )
 
 
 
# Create the form with simple fields
form = MultiPartForm()
form.add_field( 'uri', '/snapshot.cgi' )
form.add_field( 'username', webUser )
form.add_field( 'password', webPass )
  
# Our Cookie Jar
cj = cookielib.CookieJar()
opener = urllib2.build_opener( urllib2.HTTPCookieProcessor( cj ) )
 
# Just open the default url to grab the cookies and put them in the jar
print "[+] Opening default page [http://%s] to store cookies" % rhost
resp = opener.open( "http://%s" %rhost )
 
# Create our multi-part body + headers login POST request
print "[+] Logging in with user [%s] and password [%s] at host [%s]" % ( webUser, webPass, rhost )
resp = urllib2.Request( "http://%s/login.cgi" % rhost )
body = str( form )
resp.add_header( 'Content-type', form.get_content_type() )
resp.add_header( 'Content-length', len( body ) )
resp.add_data( body )
request = opener.open( resp ).read()

 
file = open("AircamImage.jpg",'w')
file.write(request)

