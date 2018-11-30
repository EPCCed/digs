package uk.ac.ed.epcc.digs.client;

import uk.ac.ed.epcc.digs.DigsException;

import java.net.URL;
import java.io.File;
import org.globus.axis.util.Util;

import gov.lbl.srm.StorageResourceManager.*;

public class SRMAdaptor implements StorageElementInterface {

    public SRMAdaptor(String ep) {
	endpoint = ep;
    }

    /*
     * SRM utility methods
     */
    protected String endpoint;

    protected static boolean firstTime = true;

    protected ISRM getSRM() throws Exception {
	if (firstTime) {
	    Util.registerTransport();
	    firstTime = false;
	}
	SRMServiceLocator loc = new SRMServiceLocator();
	ISRM srm = loc.getsrm(new URL(endpoint));
	return srm;
    }

    protected String getSrmPath(String server, String path) {
	return "srm://" + server + path;
    }

    protected boolean isSuccessCode(TStatusCode status) {
	if ((status == TStatusCode.SRM_SUCCESS) ||
	    (status == TStatusCode.SRM_REQUEST_QUEUED) ||
	    (status == TStatusCode.SRM_REQUEST_INPROGRESS) ||
	    (status == TStatusCode.SRM_REQUEST_SUSPENDED) ||
	    (status == TStatusCode.SRM_RELEASED) ||
	    (status == TStatusCode.SRM_FILE_PINNED) ||
	    (status == TStatusCode.SRM_FILE_IN_CACHE) ||
	    (status == TStatusCode.SRM_SPACE_AVAILABLE) ||
	    (status == TStatusCode.SRM_LOWER_SPACE_GRANTED) ||
	    (status == TStatusCode.SRM_DONE) ||
	    (status == TStatusCode.SRM_PARTIAL_SUCCESS)) {
	    return true;
	}
	return false;
    }

    protected String srmErrorString(TReturnStatus stat) {
	String explanation = stat.getExplanation();
	String code = stat.getStatusCode().toString();
	if (explanation == null) {
	    return code;
	}
	return code + ": " + explanation;
    }

    /*
     * gsiftp URL parsing methods
     */
    protected boolean urlIsValid(String url) {
	if (url.startsWith("gsiftp://")) {
	    return true;
	}
	return false;
    }

    protected String getGridftpServer(String url) {
	int slashidx = url.indexOf('/', 9);
	if (slashidx < 0) {
	    slashidx = url.length();
	}
	int colonidx = url.indexOf(':', 9);
	if ((colonidx >= 0) && (colonidx < slashidx)) {
	    slashidx = colonidx;
	}
	return url.substring(9, slashidx);
    }

    protected short getGridftpPort(String url) {
	int slashidx = url.indexOf('/', 9);
	if (slashidx < 0) {
	    slashidx = url.length();
	}
	int colonidx = url.indexOf(':', 9);
	if ((colonidx >= 0) && (colonidx < slashidx)) {
	    String portstr = url.substring(colonidx+1, slashidx);
	    return Short.parseShort(portstr);
	}
	return 2811;
    }

    protected String getGridftpPath(String url) {
	int slashidx = url.indexOf('/', 9);
	if (slashidx < 0) {
	    return "";
	}
	return url.substring(slashidx);
    }

    /*
     * Storage element interface methods
     */
    public void getFile(String server, String remoteFilename, String localFilename,
			TransferMonitor tm) throws Exception {
	if (endpoint == null) {
	    throw new DigsException("SRM node " + server + " is missing endpoint property");
	}

	ISRM srm = getSRM();
	String path = getSrmPath(server, remoteFilename);

	/* call srmPrepareToGet */
	TGetFileRequest[] reqs = new TGetFileRequest[1];
	reqs[0] = new TGetFileRequest(null, new org.apache.axis.types.URI(path));
	SrmPrepareToGetRequest req =
	    new SrmPrepareToGetRequest(new ArrayOfTGetFileRequest(reqs),
				       null, null, null, null, null,
				       null, null, null, null);
	SrmPrepareToGetResponse resp = srm.srmPrepareToGet(req);
	
	/* check whether it succeeded or not */
	String token = resp.getRequestToken();
	if ((!isSuccessCode(resp.getReturnStatus().getStatusCode())) ||
	    (token == null)) {
	    throw new DigsException("Error getting file from SRM: " +
				    srmErrorString(resp.getReturnStatus()));
	}

	/* keep checking request status until file is ready */
	org.apache.axis.types.URI[] uris = new org.apache.axis.types.URI[1];
	uris[0] = new org.apache.axis.types.URI(path);
	ArrayOfAnyURI aoau = new ArrayOfAnyURI(uris);
	String url = null;
	do {
	    Thread.sleep(1000);

	    SrmStatusOfGetRequestRequest req2 =
		new SrmStatusOfGetRequestRequest(aoau, null, token);
	    SrmStatusOfGetRequestResponse resp2 = srm.srmStatusOfGetRequest(req2);
	    if (!isSuccessCode(resp2.getReturnStatus().getStatusCode())) {
		throw new DigsException("Error getting file from SRM: " +
					srmErrorString(resp2.getReturnStatus()));
	    }

	    TGetRequestFileStatus stat = resp2.getArrayOfFileStatuses().getStatusArray()[0];
	    if (!isSuccessCode(stat.getStatus().getStatusCode())) {
		throw new DigsException("Error getting file from SRM" +
					srmErrorString(stat.getStatus()));
	    }
	    if (stat.getTransferURL() != null) {
		url = stat.getTransferURL().toString();
	    }
	} while (url == null);

	/* transfer file via GridFTP */
	if (!urlIsValid(url)) {
	    throw new DigsException("SRM server returned a non-gsiftp URL: " + url);
	}
	DigsFTPClient ftp = new DigsFTPClient();
	ftp.setPort(getGridftpPort(url));
	ftp.getFile(getGridftpServer(url), getGridftpPath(url), localFilename, tm);
    }

    public void putFile(String server, String localFilename, String remoteFilename,
			TransferMonitor tm) throws Exception {
	if (endpoint == null) {
	    throw new DigsException("SRM node " + server + " is missing endpoint property");
	}

	ISRM srm = getSRM();
	String path = getSrmPath(server, remoteFilename);

	long size = new File(localFilename).length();

	/* call srmPrepareToPut */
	TPutFileRequest[] reqs = new TPutFileRequest[1];
	reqs[0] = new TPutFileRequest(new org.apache.axis.types.UnsignedLong(size),
				      new org.apache.axis.types.URI(path));
	SrmPrepareToPutRequest req =
	    new SrmPrepareToPutRequest(new ArrayOfTPutFileRequest(reqs),
				       null, null, null, null, null,
				       null, null, null, null, null,
				       null);
	SrmPrepareToPutResponse resp = srm.srmPrepareToPut(req);

	/* check whether it succeeded or not */
	String token = resp.getRequestToken();
	if ((!isSuccessCode(resp.getReturnStatus().getStatusCode())) ||
	    (token == null)) {
	    throw new DigsException("Error putting file to SRM: " +
				    srmErrorString(resp.getReturnStatus()));
	}

	/* keep checking request status until file is ready */
	org.apache.axis.types.URI[] uris = new org.apache.axis.types.URI[1];
	uris[0] = new org.apache.axis.types.URI(path);
	ArrayOfAnyURI aoau = new ArrayOfAnyURI(uris);
	String url = null;
	do {
	    Thread.sleep(1000);

	    SrmStatusOfPutRequestRequest req2 =
		new SrmStatusOfPutRequestRequest(aoau, null, token);
	    SrmStatusOfPutRequestResponse resp2 = srm.srmStatusOfPutRequest(req2);
	    if (!isSuccessCode(resp2.getReturnStatus().getStatusCode())) {
		throw new DigsException("Error putting file to SRM: " + 
					srmErrorString(resp2.getReturnStatus()));
	    }

	    TPutRequestFileStatus stat = resp2.getArrayOfFileStatuses().getStatusArray()[0];
	    if (!isSuccessCode(stat.getStatus().getStatusCode())) {
		throw new DigsException("Error putting file to SRM: " +
					srmErrorString(stat.getStatus()));
	    }
	    if (stat.getTransferURL() != null) {
		url = stat.getTransferURL().toString();
	    }
	} while (url == null);

	/* transfer file via GridFTP */
	if (!urlIsValid(url)) {
	    throw new DigsException("SRM server returned a non-gsiftp URL: " +
				    url);
	}
	DigsFTPClient ftp = new DigsFTPClient();
	ftp.setLock(false);
	ftp.setPort(getGridftpPort(url));
	ftp.putFile(getGridftpServer(url), localFilename, getGridftpPath(url), tm);

	/* call srmPutDone */
	SrmPutDoneRequest req3 = new SrmPutDoneRequest(aoau, null, token);
	SrmPutDoneResponse resp3 = srm.srmPutDone(req3);
	if (!isSuccessCode(resp3.getReturnStatus().getStatusCode())) {
	    throw new DigsException("Error putting file to SRM: " +
				    srmErrorString(resp3.getReturnStatus()));
	}
    }
};
