# ***********************************************************************
#
#   Filename:   createGridmapFile.py
#
#   Authors:    Radek Ostrowski            (radek)   EPCC.
#
#   Purpose:    Creates a grid-mapfile by merging a local grid-mapfile with
#               a list of users of the VOMS service
#
#   Contents:   Python script
#
#   Used in:    Security for QCDgrid
#
#   Contact:    epcc-support@epcc.ed.ac.uk
#
#   Copyright (c) 2010 The University of Edinburgh
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
#   MA 02111-1307, USA.
#
#   As a special exception, you may link this program with code
#   developed by the OGSA-DAI project without such code being covered
#   by the GNU General Public License.
#
#  ***********************************************************************

import string
import urllib
import xml.sax
import dnhandler
import shutil
import smtplib

def configure():
    config = {}
    config['email'] = 'radek@epcc.ed.ac.uk'
    config['ukqcd'] = 'ukqcd' #desired mapping in the grid-mapfile for UKQCD members
    config['ildg'] = 'ildg' #desired mapping in the grid-mapfile for ILDG members
    config['gridmapfile'] = '/etc/grid-security/grid-mapfile'
    config['gridmapfilelocal'] = '/etc/grid-security/grid-mapfile-local'
    config['gridmapfileold'] = '/etc/grid-security/grid-mapfile-old'
    config['key'] = '/etc/grid-security/hostkey.pem'
    config['cert'] = '/etc/grid-security/hostcert.pem'

    return config
    
def sendEmail(config, message):
    fromaddr = "digs@epcc.ed.ac.uk"
    toaddrs  = config['email']

    subject = "New Users Added to ILDG VOMS"
    
    msg = ("From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n"
                  % (fromaddr, toaddrs, subject))

    msg += message

    server = smtplib.SMTP('localhost')
    server.sendmail(fromaddr, toaddrs, msg)
    server.quit()

def getXML(config, group):

  if group == "ildg":
    url_group = ""
  else:
    url_group = "/" + group
  
  key = config['key']
  cert = config['cert']
  
  url = 'https://grid-voms.desy.de:8443/voms/ildg/services/VOMSCompatibility?method=getGridmapUsers&container=/ildg' + url_group
  temp_xml = group + '.xml'

  opener = urllib.URLopener(key_file = key, cert_file = cert)
  opener.retrieve(url, temp_xml)


def parseXML(config, group):
  parser = xml.sax.make_parser(  )
  handler = dnhandler.DNHandler()
  parser.setContentHandler(handler)

  #downlaod the file
  getXML(config, group)

  parser.parse(group + ".xml")
  
  return handler.list

##################################################
#main:

conf = configure()
gridmapfile = conf['gridmapfile']
gridmapfilelocal = conf['gridmapfilelocal']
gridmapfileold = conf['gridmapfileold']

ukqcd = set(parseXML(conf, "ukqcd"))
ildg = set(parseXML(conf, "ildg"))

ildg = ildg - ukqcd

localDNs = []
# add entries from grid-mapfile-local
localFile = open(gridmapfilelocal, 'r')
localDNs = localFile.readlines()
localFile.close()

newDNs = localDNs

isLocal=0
for dn in ukqcd:
  for existingDN in newDNs:
      localDN = string.split(existingDN, '"')
      if string.count(localDN[1], dn) == 1:
         isLocal=1
         break
      else:
         isLocal=0
  if isLocal==0:
      newDNs.append('"' + dn + '" ' + conf['ukqcd'] + "\n")
      
isLocal=0
for dn in ildg:
  for existingDN in newDNs:
      localDN = string.split(existingDN, '"')
      if string.count(localDN[1], dn) == 1:
         isLocal=1
         break
      else:
          isLocal=0    
  if isLocal==0:
      newDNs.append('"' + dn + '" ' + conf['ildg'] + "\n")
            
currentFile = open(gridmapfile, 'r')
currentDNs = currentFile.readlines()
currentFile.close()

#compare the differences and send email
currentSet = set(currentDNs)
newSet = set(newDNs)
differences = newSet - currentSet

if len(differences) > 0:
    sendEmail(conf, "".join(differences))

# rename existing to OLD
shutil.move(gridmapfile, gridmapfileold)

# and new to gridmap-file
newFile = open(gridmapfile, 'w')
newFile.writelines(newDNs)

newFile.close()
