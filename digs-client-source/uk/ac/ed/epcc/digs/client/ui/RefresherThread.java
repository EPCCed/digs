/*
 * Thread that runs in the background and performs periodic tasks for the GUI:
 *  1. Refreshing the proxy status panel every minute
 *  2. Refreshing the grid file tree every 5 minutes
 * Timings are not accurate but they shouldn't need to be
 */
package uk.ac.ed.epcc.digs.client.ui;

public class RefresherThread implements Runnable {

    private Main main;
    private Thread thread;

    private boolean stopping;

    private int proxyIterations;
    private int gridTreeIterations;
    private int localTreeIterations;

    public void run() {
        proxyIterations = 60;
        gridTreeIterations = 300;
	localTreeIterations = 30;

        while (!stopping) {
            // wait one second
            try {
                Thread.sleep(1000);
            }
            catch (Exception ex) {
            }

            proxyIterations--;
            if (proxyIterations == 0) {
                // update the proxy status if time
                main.updateProxyStatus();
                proxyIterations = 60;
            }
            
            gridTreeIterations--;
            if (gridTreeIterations == 0) {
                // update the grid tree status if time
                main.refreshGridTree(true);
                gridTreeIterations = 300;
            }

	    localTreeIterations--;
	    if (localTreeIterations == 0) {
		// update the local tree status if time
		main.refreshLocalTree();
		localTreeIterations = 30;
	    }
        }
    }

    public RefresherThread(Main mn) {
        // start up the refresher thread
        main = mn;
        stopping = false;
        thread = new Thread(this);
        thread.start();
    }

    public void stop() {
        // tell the thread to stop
        stopping = true;
    }
};
