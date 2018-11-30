package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

import java.util.Vector;
import java.io.File;

import uk.ac.ed.epcc.digs.client.DigsClient;
import uk.ac.ed.epcc.digs.client.LogicalFile;
import uk.ac.ed.epcc.digs.client.LogicalDirectory;

public class TransferTable implements MouseListener, ActionListener, ComponentListener {

    // Allows us to render error messages in bright red
    class TransferTableRenderer extends DefaultTableCellRenderer {
        public TransferTableRenderer() {
            super();
        }

        public Component getTableCellRendererComponent(
            JTable table, Object value,
            boolean isSelected, boolean hasFocus,
            int row, int column) {

            String str = (String) value;

            if ((column == 3) && (str.startsWith("Error")))  {
                setForeground(new Color(255, 0, 0));
            }
            else {
                setForeground(new Color(0, 0, 0));
            }
            return super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
        } 
    }

    class TransferTableModel extends AbstractTableModel {
        private TransferTable transferTable;

        public TransferTableModel(TransferTable tt) {
            transferTable = tt;
        }

        public int getRowCount() {
            return transferTable.getTransfers().size();
        }

        public int getColumnCount() {
            return 4;
        }

        public Object getValueAt(int row, int column) {
            String val = "";

            Vector xfers = transferTable.getTransfers();
            FileTransfer ft = (FileTransfer)xfers.elementAt(row);

            switch (column)
            {
            case 0:
                val = ft.getSource();
                break;
	    case 1:
		val = ft.getDestination();
		break;
            case 2:
		if (ft.isRefresh()) {
		    val = "Refresh";
		}
		else {
		    if (ft.isPutTransfer()) {
			val = "Upload";
		    }
		    else {
			val = "Download";
		    }
		}
                break;
            case 3:
                val = ft.getStatusString();
                break;
            }

            return val;
        }

        public String getColumnName(int column) {
            String name = "";
            switch (column)
            {
            case 0:
                name = "File";
                break;
	    case 1:
		name = "Destination";
		break;
            case 2:
                name = "Type";
                break;
            case 3:
                name = "Status";
                break;
            }
            return name;
        }
    }

    private JTable jtable;
    private JScrollPane scrollpane;
    private TransferTableModel model;
    private DigsClient digsClient;
    private int maxParallelTransfers;

    private Vector transfers;

    private JMenuItem cancelItem;

    public TransferTable(DigsClient dc, int mpt) {
        digsClient = dc;
        maxParallelTransfers = mpt;
        transfers = new Vector();

        cancelItem = new JMenuItem("Cancel");
        cancelItem.addActionListener(this);

        model = new TransferTableModel(this);
        jtable = new JTable(model);
        jtable.addMouseListener(this);
        jtable.setDefaultRenderer(Object.class, new TransferTableRenderer());
        jtable.addComponentListener(this);

        scrollpane = new JScrollPane();
        scrollpane.setPreferredSize(new Dimension(400, 100));
        scrollpane.getViewport().add(jtable);

        rowClicked = -1;
    }

    Vector getTransfers() {
        return transfers;
    }

    public JTable getJTable() {
        return jtable;
    }

    public JScrollPane getScrollPane() {
        return scrollpane;
    }

    private void scrollToBottom() {
        JScrollBar scrollbar = scrollpane.getVerticalScrollBar();
        if (scrollbar != null) {
            scrollbar.setValue(scrollbar.getMaximum());
        }
    }

    public void tryStartTransfers() {
        int transfersRunning = 0;
        int i;

        // count current transfers
        for (i = 0; i < transfers.size(); i++) {
            FileTransfer ft = (FileTransfer) transfers.elementAt(i);
            int status = ft.getStatus();
            if ((status == FileTransfer.UPLOADING) || (status == FileTransfer.DOWNLOADING)) {
                transfersRunning++;
            }
        }

        for (i = 0; i < transfers.size(); i++) {
            if (transfersRunning >= maxParallelTransfers) break;

            FileTransfer ft = (FileTransfer) transfers.elementAt(i);
            int status = ft.getStatus();
            if (status == FileTransfer.WAITING) {
                ft.start();
                transfersRunning++;
            }
        }
    }

    private String addPreAndPostFixes(String src, String prefix, String postfix, String separator) {
        String result;
        int nameStart = src.lastIndexOf(separator);
        int nameEnd = src.lastIndexOf(".");

        if (nameStart >= 0) {
            result = src.substring(0, nameStart+1) + prefix;
        }
        else {
            result = prefix;
        }

        if (nameEnd >= 0) {
            result = result + src.substring(nameStart+1, nameEnd) + postfix + src.substring(nameEnd);
        }
        else {
            result = result + src.substring(nameStart+1) + postfix;
        }

        return result;
    }

    private int replaceStatus;

    private void addFilePut(File file, String dest, String prefix, String postfix) {
        boolean exists = false;

        /* Add directory path and prefix and postfix here */
        String wpf = addPreAndPostFixes(file.getName(), prefix, postfix, File.separator);

        String thisDest;
        if (dest.equals("")) {
            thisDest = wpf;
        }
        else {
            thisDest = dest + "/" + wpf;
        }
        
        System.out.println("Putting " + file.getPath() + " to " + thisDest);

        /* Check if destination file already exists */
        exists = digsClient.fileExists(thisDest);
        if (exists) {

            /* skip if it exists and we already got "No to all" from user */
            if (replaceStatus == ReplaceFileDialogue.NO_TO_ALL) return;
            
            if (replaceStatus != ReplaceFileDialogue.YES_TO_ALL) {
                /* ask whether to replace */
                ReplaceFileDialogue rfd = new ReplaceFileDialogue(null, thisDest);
                int result = rfd.getResult();
                if ((result == ReplaceFileDialogue.YES_TO_ALL) || (result == ReplaceFileDialogue.NO_TO_ALL)) {
                    replaceStatus = result;
                }

                if ((result == ReplaceFileDialogue.NO) || (result == ReplaceFileDialogue.NO_TO_ALL)) {
                    return;
                }
            }
        }
        
        /* Create transfer object */
        FileTransfer xfer = new FileTransfer(file.getPath(), thisDest, digsClient, true, this, file.length());
        transfers.add(xfer);
    }

    private void addDirectoryPuts(File dir, String dest, String prefix, String postfix) {
        if (dest.equals("")) {
            dest = dir.getName();
        }
        else {
            dest = dest + "/" + dir.getName();
        }

        /* Iterate over all files in directory */
        File[] files = dir.listFiles();
        for (int i = 0; i < files.length; i++) {

            if (files[i].isDirectory()) {
                /* Call recursively for a subdirectory */
                addDirectoryPuts(files[i], dest, prefix, postfix);
            }
            else if (files[i].isFile()) {
                addFilePut(files[i], dest, prefix, postfix);
            }
        }
    }

    /*
     * srcs is a vector of Files, which may be regular files or directories
     * dest is the logical name of the destination directory on the grid
     */
    public void addPutTransfer(Vector srcs, String dest, String prefix, String postfix) {
        int firstidx = transfers.size();

        /* reset file replacement status for this block of transfers */
        replaceStatus = -1;

        /* Iterate over all sources */
        for (int i = 0; i < srcs.size(); i++) {

            /* Work out the full LFN and PFN for each one */
            File file = (File)srcs.elementAt(i);

            if (!file.isDirectory()) {
                addFilePut(file, dest, prefix, postfix);
            }
            else {
                /* Expand directory sources into individual files */
                addDirectoryPuts(file, dest, prefix, postfix);
            }
        }

        model.fireTableRowsInserted(firstidx, transfers.size()-1);

        tryStartTransfers();
    }

    private void addDirectoryGets(LogicalDirectory ld, String dest, String prefix, String postfix) {
        dest = dest + File.separator + ld.getName();
        try {
            Vector v = ld.getFiles();
            for (int i = 0; i < v.size(); i++) {
                LogicalFile lf = (LogicalFile) v.elementAt(i);
                if (lf.isDirectory()) {
                    addDirectoryGets((LogicalDirectory)lf, dest, prefix, postfix);
                }
                else {
                    addFileGet(lf, dest, prefix, postfix);
                }
            }

        }
        catch (Exception ex) {
            // FIXME: deal with somehow
            System.err.println("Error getting file list: " + ex);
        }
    }

    private void addFileGet(LogicalFile lf, String dest, String prefix, String postfix) {
        String wpf = addPreAndPostFixes(lf.getName(), prefix, postfix, "/");

        String thisDest = dest + File.separator + wpf;

        System.out.println("Getting " + lf.getPath() + " to " + thisDest);

        FileTransfer xfer = new FileTransfer(lf.getPath(), thisDest, digsClient, false, this, lf.getSize());
        transfers.add(xfer);
    }

    /*
     * srcs is a vector of LogicalFiles which may be regular files or directories
     * dest is the local name of the destination directory
     */
    public void addGetTransfer(Vector srcs, String dest, String prefix, String postfix) {
        int firstidx = transfers.size();

        /* Iterate over all sources */
        for (int i = 0; i < srcs.size(); i++) {

            LogicalFile lf = (LogicalFile)srcs.elementAt(i);
            System.out.println("Getting " + lf.getPath() + " to " + dest);

            if (lf.isDirectory()) {
                /* Expand directory sources into individual files */
                addDirectoryGets((LogicalDirectory)lf, dest, prefix, postfix);
            }
            else {
                addFileGet(lf, dest, prefix, postfix);
            }
        }

        model.fireTableRowsInserted(firstidx, transfers.size()-1);

        tryStartTransfers();
    }

    public void addRefresh() {
	int firstidx = transfers.size();
	FileTransfer xfer = new FileTransfer(this);
	transfers.add(xfer);
	model.fireTableRowsInserted(firstidx, transfers.size()-1);
	xfer.start();
    }

    public void statusChanged() {
        model.fireTableDataChanged();

        tryStartTransfers();
    }

    private int rowClicked;

    /*
     * Mouse listener methods
     */
    public void mouseClicked(MouseEvent e) {
        /* respond to right clicks */
        if (e.getButton() == MouseEvent.BUTTON3) {
            Point pt = new Point(e.getX(), e.getY());
            int idx = jtable.rowAtPoint(pt);
            rowClicked = idx;

            FileTransfer ft = (FileTransfer) transfers.elementAt(idx);
            int state = ft.getStatus();
            if ((state == FileTransfer.WAITING) || (state == FileTransfer.UPLOADING) ||
                (state == FileTransfer.DOWNLOADING)) {
                cancelItem.setEnabled(true);
            }
            else {
                cancelItem.setEnabled(false);
            }
            
            JPopupMenu popup = new JPopupMenu();
            popup.add(cancelItem);
            popup.show(jtable, e.getX(), e.getY());

	    // If clicked on a non-selected row, clear any that are selected
	    boolean isselected = false;
	    int[] selections = jtable.getSelectedRows();
	    if (selections != null) {
		for (int i = 0; i < selections.length; i++) {
		    if (selections[i] == rowClicked) {
			isselected = true;
		    }
		}
		if (!isselected) {
		    jtable.clearSelection();
		}
	    }
        }
    }

    public void mouseEntered(MouseEvent e) {
    }
    
    public void mouseExited(MouseEvent e) {
    }

    public void mousePressed(MouseEvent e) {
    }

    public void mouseReleased(MouseEvent e) {
    }

    public void actionPerformed(ActionEvent e) {
        Object src = e.getSource();
        if (src == cancelItem) {
            // cancel the transfer(s)

            int[] rows = jtable.getSelectedRows();
            if (rows.length == 0) {
                FileTransfer ft = (FileTransfer) transfers.elementAt(rowClicked);
                int status = ft.getStatus();
                if ((status == FileTransfer.WAITING) || (status == FileTransfer.UPLOADING) ||
                    (status == FileTransfer.DOWNLOADING)) {
                    ft.cancel();
                }
            }
            else {
                for (int i = 0; i < rows.length; i++) {
                    FileTransfer ft = (FileTransfer) transfers.elementAt(rows[i]);
                    int status = ft.getStatus();
                    if ((status == FileTransfer.WAITING) || (status == FileTransfer.UPLOADING) ||
                        (status == FileTransfer.DOWNLOADING)) {
                        ft.cancel();
                    }
                }
            }
        }
    }

    public void componentHidden(ComponentEvent e) {
    }

    public void componentMoved(ComponentEvent e) {
    }

    public void componentResized(ComponentEvent e) {
        /*
         * Scroll to show the most recent entry every time the table expands
         */
        scrollToBottom();
    }

    public void componentShown(ComponentEvent e) {
    }
}
