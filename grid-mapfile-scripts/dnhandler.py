import xml.sax.handler

class DNHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.inDN = 0
        self.list = []
        
    def startElement(self, name, attributes):
        if name == "getGridmapUsersReturn":
          if attributes.getValue('xsi:type') == "soapenc:string":
            self.inDN = 1
          else:
            self.inDN = 0

    def characters(self, data):
        if self.inDN == 1:
            self.list.append(data)

#    def endElement(self, name):



