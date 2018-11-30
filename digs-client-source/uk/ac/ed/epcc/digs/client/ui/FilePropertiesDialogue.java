/*
 * Simple modal dialogue which shows properties for a file or folder (grid or local)
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import java.io.File;
import java.util.Vector;

import uk.ac.ed.epcc.digs.client.LogicalFile;

public class FilePropertiesDialogue extends JDialog implements ActionListener {

    private JButton okButton;

    private JLabel iconLabel;
    private JLabel nameLabel;

    private JLabel sizeLabel;
    private JLabel ownerLabel;
    private JLabel groupLabel;
    private JLabel permissionsLabel;
    private JLabel checksumLabel;
    private JLabel locationsLabel;

    private JLabel lockedbyLabel;
    private JLabel replcountLabel;

    private JPanel buttonPanel;

    /*
     * Recursively adds up the size of all files in a folder
     */
    private long getDirectorySize(File file) {
        long total = 0;

        File[] files = file.listFiles();
        if (files != null) {
            for (int i = 0; i < files.length; i++) {
                if (files[i].isDirectory()) {
                    total += getDirectorySize(files[i]);
                }
                else if (files[i].isFile()) {
                    total += files[i].length();
                }
            }
        }

        return total;
    }

    /*
     * This constructor creates a properties dialogue for a local file or directory
     * Just shows an icon, the name and the size
     */
    public FilePropertiesDialogue(Frame parent, File file, String imgpath) {
        super(parent, "Properties", true);

        okButton = new JButton("OK");
        okButton.addActionListener(this);

        // get icon for file or directory, and calculate size
        ImageIcon icon;
        long size;
        if (file.isDirectory()) {
            size = getDirectorySize(file);
            icon = new ImageIcon(imgpath + File.separator + "icon-folder.png");
        }
        else {
            size = file.length();
            icon = new ImageIcon(imgpath + File.separator + "icon-file.png");
        }

        iconLabel = new JLabel(icon);
        nameLabel = new JLabel(file.getName());

        sizeLabel = new JLabel("Size: ");

        this.getContentPane().setLayout(new GridLayout(3, 2));

        this.getContentPane().add(iconLabel);
        this.getContentPane().add(nameLabel);
        this.getContentPane().add(sizeLabel);
        this.getContentPane().add(new JLabel("" + size));

        buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(okButton);

        this.getContentPane().add(buttonPanel);

        pack();
        setLocationRelativeTo(null);
        setVisible(true);
    }

    /*
     * This constructor creates a properties dialogue for a grid file or directory
     * Always shows icon, name and size. For files it additionally shows owner, group,
     * permissions and checksum. For locked files the user holding the lock is shown
     * Replication count is shown if it differs from the default
     */
    public FilePropertiesDialogue(Frame parent, LogicalFile lf, String imgpath) {
        super(parent, "Properties", true);

        okButton = new JButton("OK");
        okButton.addActionListener(this);

        ImageIcon icon;
        int gridsize;
        String lockedby = null;
        int replcount = 0;
        if (lf.isDirectory()) {
            icon = new ImageIcon(imgpath + File.separator + "icon-folder.png");
            gridsize = 3;
        }
        else {
            icon = new ImageIcon(imgpath + File.separator + "icon-file.png");

            replcount = lf.getReplicaCount();

            lockedby = lf.getLockedBy();

            gridsize = 8;
            if (replcount > 0) {
                gridsize++;
            }
            if (lockedby != null) {
                gridsize++;
            }
        }
        GridLayout layout = new GridLayout(gridsize, 2);
        iconLabel = new JLabel(icon);
        nameLabel = new JLabel(lf.getName());

        sizeLabel = new JLabel("Size: ");
        ownerLabel = new JLabel("Owner: ");
        groupLabel = new JLabel("Group: ");
        permissionsLabel = new JLabel("Permissions: ");
        checksumLabel = new JLabel("Checksum: ");
        locationsLabel = new JLabel("Locations: ");

        lockedbyLabel = new JLabel("Locked by: ");
        replcountLabel = new JLabel("Replication count: ");

        this.getContentPane().setLayout(layout);

        this.getContentPane().add(iconLabel);
        this.getContentPane().add(nameLabel);
        this.getContentPane().add(sizeLabel);
        this.getContentPane().add(new JLabel("" + lf.getSize()));
        
        if (!lf.isDirectory()) {
            String owner = lf.getOwner();
            JLabel ol;
            if (owner == null) {
                owner = "<unknown>";
            }
            if (owner.length() > 32) {
                /*
                 * Stop the dialogue getting ridiculously big with a long owner
                 * string. Show the full string in a tool tip
                 */
                ol = new JLabel(owner.substring(0, 30) + "...");
                ol.setToolTipText(owner);
            }
            else {
                ol = new JLabel(owner);
            }
            this.getContentPane().add(ownerLabel);
            this.getContentPane().add(ol);

            String group = lf.getGroup();
            if (group == null) {
                group = "<unknown>";
            }
            this.getContentPane().add(groupLabel);
            this.getContentPane().add(new JLabel(group));

            String perms = lf.getPermissions();
            if (perms == null) {
                perms = "<unknown>";
            }
            this.getContentPane().add(permissionsLabel);
            this.getContentPane().add(new JLabel(perms));

            String checksum = lf.getChecksum();
            if (checksum == null) {
                checksum = "<unknown>";
            }
            this.getContentPane().add(checksumLabel);
            this.getContentPane().add(new JLabel(checksum));

            Vector locations = lf.getLocations();
            String locstr = "";
            String fulllocstr = "";
            if (locations == null) {
                locstr = "<unknown>";
                fulllocstr = "<unknown>";
            }
            else {
                for (int i = 0; i < locations.size(); i++) {
                    fulllocstr = fulllocstr + (String)locations.elementAt(i);
                    if (i < (locations.size() - 1)) {
                        fulllocstr = fulllocstr + ", ";
                    }
                }
                if (fulllocstr.length() > 32) {
                    locstr = fulllocstr.substring(0, 30) + "...";
                }
                else {
                    locstr = fulllocstr;
                }
            }
            JLabel ll = new JLabel(locstr);
            ll.setToolTipText(fulllocstr);
            this.getContentPane().add(locationsLabel);
            this.getContentPane().add(ll);

            if (lockedby != null) {
                JLabel lbl;
                if (lockedby.length() > 32) {
                    /*
                     * Stop the dialogue getting ridiculously big with a long locked
                     * by string. Show the full string in a tool tip
                     */
                    lbl = new JLabel(lockedby.substring(0, 30) + "...");
                    lbl.setToolTipText(lockedby);
                }
                else {
                    lbl = new JLabel(lockedby);
                }
                this.getContentPane().add(lockedbyLabel);
                this.getContentPane().add(lbl);
            }
            if (replcount > 0) {
                this.getContentPane().add(replcountLabel);
                this.getContentPane().add(new JLabel("" + replcount));
            }
        }

        buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(okButton);

        this.getContentPane().add(buttonPanel);

        pack();
        setLocationRelativeTo(null);
        setVisible(true);
    }


    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == okButton) {
            // close dialogue when ok pushed
            dispose();
        }
    }
}
