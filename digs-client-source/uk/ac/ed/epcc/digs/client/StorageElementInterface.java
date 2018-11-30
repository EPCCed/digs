package uk.ac.ed.epcc.digs.client;

public interface StorageElementInterface {
    public void getFile(String server, String remoteFilename, String localFilename,
			TransferMonitor tm) throws Exception;
    public void putFile(String server, String localFilename, String remoteFilenaem,
			TransferMonitor tm) throws Exception;
};
