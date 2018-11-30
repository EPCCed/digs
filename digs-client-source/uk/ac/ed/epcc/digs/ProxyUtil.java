package uk.ac.ed.epcc.digs;

import java.security.cert.X509Certificate;
import java.security.PrivateKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.io.File;
import java.io.FileOutputStream;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Date;
import java.util.Observer;
import java.util.Observable;
import java.util.Vector;
import java.util.Map;
import java.util.HashMap;

import org.globus.common.CoGProperties;
import org.globus.gsi.GlobusCredential;
import org.globus.gsi.CertUtil;
import org.globus.util.Util;
import org.globus.util.ConfigUtil;
import org.globus.gsi.OpenSSLKey;
import org.globus.gsi.bc.BouncyCastleOpenSSLKey;
import org.globus.gsi.bc.BouncyCastleCertProcessingFactory;
import org.globus.gsi.X509ExtensionSet;
import org.globus.gsi.proxy.ext.ProxyPolicy;
import org.globus.gsi.proxy.ProxyPolicyHandler;
import org.globus.gsi.proxy.ext.ProxyCertInfo;
import org.globus.gsi.proxy.ext.ProxyCertInfoExtension;
import org.globus.gsi.proxy.ext.GlobusProxyCertInfoExtension;
import org.globus.gsi.GSIConstants;
import org.globus.gsi.TrustedCertificates;
import org.globus.gsi.proxy.ProxyPathValidator;
import org.globus.gsi.proxy.ProxyPathValidatorException;

import org.glite.voms.contact.VOMSProxyInit;
import org.glite.voms.contact.VOMSRequestOptions;

public class ProxyUtil {

    /*
     * Gets the list of supported virtual organisations from the VOMSES directory
     */
    public static Object[] getVOList() {
	Vector vos = new Vector();
	int i;
	vos.add("None");

	String vomsesLoc = System.getProperty("VOMSES_LOCATION");
	if ((vomsesLoc != null) && (!vomsesLoc.equals(""))) {
	    File vomsesDir = new File(vomsesLoc + File.separator + "vomses");
	    if (vomsesDir.isDirectory()) {
		File[] vomsesFiles = vomsesDir.listFiles();
		for (i = 0; i < vomsesFiles.length; i++) {
		    try {
			BufferedReader reader = new BufferedReader(new FileReader(vomsesFiles[i]));
			String line = reader.readLine();
			reader.close();

			/* VO name is the first item on the line. It can be double-quoted */
			int start, end;
			if (line.charAt(0) == '"') {
			    start = 1;
			    end = line.indexOf('"', 1);
			}
			else {
			    start = 0;
			    end = line.indexOf(' ');
			}
			String vo = line.substring(start, end);
			vos.add(vo);
		    }
		    catch (IOException ex) {
		    }
		}
	    }
	}

	Object[] result = new Object[vos.size()];
	for (i = 0; i < vos.size(); i++) {
	    result[i] = vos.elementAt(i);
	}
	return result;
    }

    /*
     * Returns the DN from the user's proxy
     */
    public static String getProxyDN() {
        try {
            GlobusCredential proxy;
            String file = CoGProperties.getDefault().getProxyFile();
            proxy = new GlobusCredential(file);
            return proxy.getIdentity();
        }
        catch (Exception e) {
            return null;
        }
    }

    /*
     * Returns the DN from the user certificate file
     */
    public static String getCertDN() {
        try {
            X509Certificate[] certificates = CertUtil.loadCertificates(CoGProperties.getDefault().getUserCertFile());
            return CertUtil.toGlobusID(certificates[0].getSubjectDN());
        }
        catch (Exception e) {
            return null;
        }
    }

    /*
     * Returns the time in seconds that the user's proxy is valid for
     */
    public static long getProxyTimeleft() {
        try {
            GlobusCredential proxy;
            String file = CoGProperties.getDefault().getProxyFile();
            proxy = new GlobusCredential(file);
            return proxy.getTimeLeft();
        }
        catch (Exception e) {
            return 0;
        }
    }

    /*
     * Destroys the user's proxy file
     */
    public static void destroyProxy() throws Exception {
        Util.destroy(CoGProperties.getDefault().getProxyFile());
    }

    /*
     * Set the proxy permissions to 600. The CoG version of this has a bug so need to write our own.
     */
    public static void setFilePermissions() {
	if (ConfigUtil.getOS() == ConfigUtil.WINDOWS_OS) {
            return;
        }
	try {
	    Process process = null;
	    String[] cmd = new String[] { "chmod", "600", CoGProperties.getDefault().getProxyFile() };
	    process = Runtime.getRuntime().exec(cmd, null);
	    int result = process.waitFor();
	    if (result != 0) {
		System.out.println("Warning, could not set proxy file to private");
	    }
	}
	catch (Exception ex) {
	    System.out.println("Warning, could not set proxy file to private");
	}
    }

    /*
     * Checks whether the passphrase entered by the user is valid or not
     */
    public static boolean isValidPassphrase(String passphrase) {
	boolean result = true;

        /* Load private key and decrypt with passphrase */
        OpenSSLKey key;
        PrivateKey userkey;
        try {
            key = new BouncyCastleOpenSSLKey(CoGProperties.getDefault().getUserKeyFile());
            if (key.isEncrypted()) {
                key.decrypt(passphrase);
            }
            userkey = key.getPrivateKey();
        }
        catch (Exception e) {
	    result = false;
        }

	return result;
    }

    public static void createVOMSProxy(String passphrase, int hours, Observer obs, String vo) throws Exception {
	if (obs != null) {
	    obs.update(null, new Integer(0));
	}
	VOMSProxyInit proxyInit = VOMSProxyInit.instance(passphrase);
	proxyInit.setProxyOutputFile(CoGProperties.getDefault().getProxyFile());
	proxyInit.setProxyLifetime(hours * 3600);
	Map options = new HashMap();
	VOMSRequestOptions o = new VOMSRequestOptions();
	o.setVoName(vo);
	o.addFQAN("/" + vo);
	options.put(vo, o);

	if (obs != null) {
	    obs.update(null, new Integer(33));
	}
	proxyInit.getVomsProxy(options.values());
	if (obs != null) {
	    obs.update(null, new Integer(100));
	}
    }

    /*
     * Creates the type of proxy created by grid-proxy-init by default (with no special options).
     * Should work for most users, if a specific proxy type is needed it can be created by running
     * grid-proxy-init before the GUI.
     */
    public static void createProxy(String passphrase, int hours, Observer obs, String vo) throws Exception {

	if (obs != null) {
	    obs.update(null, new Integer(0));
	}

        /* Check cert and key files exist */
        File f = new File(CoGProperties.getDefault().getUserCertFile());
        if (!f.exists()) {
            throw new DigsException("User certificate file does not exist");
        }
        f = new File(CoGProperties.getDefault().getUserKeyFile());
        if (!f.exists()) {
            throw new DigsException("User key file does not exist");
        }

	/* Handle VOMS proxy creation */
	if (vo != null) {
	    createVOMSProxy(passphrase, hours, obs, vo);
	    return;
	}

        /* Load user certificate */
        X509Certificate[] certificates;
        try {
            certificates = CertUtil.loadCertificates(CoGProperties.getDefault().getUserCertFile());
        }
        catch (Exception e) {
            throw new DigsException("Error loading user certificate file");
        }

        /* Check cert is not expired */
        Date notBefore = certificates[0].getNotBefore();
        Date notAfter = certificates[0].getNotAfter();
        Date now = new Date();
        if (now.after(notAfter)) {
            throw new DigsException("User certificate has expired");
        }
        if (now.before(notBefore)) {
            throw new DigsException("User certificate is not yet valid");
        }

	if (obs != null) {
	    obs.update(null, new Integer(15));
	}

        /* Load private key and decrypt with passphrase */
        OpenSSLKey key;
        PrivateKey userkey;
        try {
            key = new BouncyCastleOpenSSLKey(CoGProperties.getDefault().getUserKeyFile());
            if (key.isEncrypted()) {
                key.decrypt(passphrase);
            }
            userkey = key.getPrivateKey();
        }
        catch (Exception e) {
            throw new DigsException("Error loading user key (possibly invalid passphrase)");
        }

	if (obs != null) {
	    obs.update(null, new Integer(30));
	}

        /* Verify that cert and key match */
        RSAPublicKey pubkey = (RSAPublicKey)certificates[0].getPublicKey();
        RSAPrivateKey privkey = (RSAPrivateKey)userkey;
        if (!pubkey.getModulus().equals(privkey.getModulus())) {
            throw new DigsException("Certificate does not match private key");
        }

	if (obs != null) {
	    obs.update(null, new Integer(40));
	}

        /* Create proxy */
        BouncyCastleCertProcessingFactory factory = BouncyCastleCertProcessingFactory.getDefault();
        X509ExtensionSet extSet = new X509ExtensionSet();
        ProxyCertInfo proxyCertInfo = new ProxyCertInfo(new ProxyPolicy(ProxyPolicy.IMPERSONATION));
        extSet.add(new GlobusProxyCertInfoExtension(proxyCertInfo));
        
        GlobusCredential proxy = factory.createCredential(certificates, userkey, 512, hours * 3600,
                                                          GSIConstants.GSI_3_IMPERSONATION_PROXY, extSet);

	if (obs != null) {
	    obs.update(null, new Integer(70));
	}

        /* Verify proxy */
        /* Verify certificate chain */
        TrustedCertificates trustedCerts = TrustedCertificates.getDefaultTrustedCertificates();
        if ((trustedCerts == null) || (trustedCerts.getCertificates() == null) ||
            (trustedCerts.getCertificates().length == 0)) {
            throw new DigsException("Unable to load CA certificates");
        }
        ProxyPathValidator validator = new ProxyPathValidator();
        String oid = proxyCertInfo.getProxyPolicy().getPolicyLanguage().getId();
        validator.setProxyPolicyHandler(oid, new ProxyPolicyHandler() {
                public void validate(ProxyCertInfo proxyCertInfo,
                                     X509Certificate[] certPath,
                                     int index)
                    throws ProxyPathValidatorException {
                }
            });
        validator.validate(proxy.getCertificateChain(),
                           trustedCerts.getCertificates());

	if (obs != null) {
	    obs.update(null, new Integer(90));
	}

        /* Write proxy to disk */
        File file = Util.createFile(CoGProperties.getDefault().getProxyFile());
	setFilePermissions();
        FileOutputStream out = new FileOutputStream(file);
        proxy.save(out);

	if (obs != null) {
	    obs.update(null, new Integer(100));
	}
    }

    public static void createProxy(String passphrase, int hours) throws Exception {
	createProxy(passphrase, hours, null, null);
    }
};
