/*
 * Simple modal dialogue which shows properties for a file in OMERO
 */
package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;

public class OMEROMetadataViewDialogue extends JDialog implements ActionListener {

    private JButton okButton;

    private JPanel buttonPanel;

    public OMEROMetadataViewDialogue(Frame parent, String filename) {
        super(parent, "OMERO Metadata", true);

        okButton = new JButton("OK");
        okButton.addActionListener(this);

        GridLayout layout = new GridLayout(11, 2);
        this.getContentPane().setLayout(layout);

        OMEROMetadata omd = null;

        try {
            OMEROInterface omero = OMEROInterface.getInstance();
            omd = omero.getMetadata(filename);
        }
        catch (Exception ex) {
            dispose();
            JOptionPane.showMessageDialog(null, ex.toString(), "Error contacting OMERO server",
                                          JOptionPane.ERROR_MESSAGE);
            return;
        }

        Border blackline = BorderFactory.createLineBorder(Color.black);


        this.getContentPane().add(new JLabel("Filename:"));
        this.getContentPane().add(new JLabel(filename));

        this.getContentPane().add(new JLabel("Description:"));
        JTextArea description = new JTextArea(omd.description);
        description.setBorder(blackline);
        description.setEditable(false);
        this.getContentPane().add(description);

        this.getContentPane().add(new JLabel("Comments:"));
        JTextArea comments = new JTextArea();
        comments.setBorder(blackline);
        comments.setEditable(false);
        if (omd.comments != null) {
            for (int i = 0; i < omd.comments.size(); i++) {
                String comment = (String)omd.comments.elementAt(i);
                comments.append(comment);
                comments.append("\n==\n");
            }
        }
        this.getContentPane().add(comments);

        this.getContentPane().add(new JLabel("Links:"));
        JTextArea links = new JTextArea();
        links.setEditable(false);
        links.setBorder(blackline);
        if (omd.links != null) {
            for (int i = 0; i < omd.links.size(); i++) {
                String link = (String)omd.links.elementAt(i);
                links.append(link);
                links.append("\n");
            }
        }
        this.getContentPane().add(links);

        this.getContentPane().add(new JLabel("Tags:"));
        String tagstr = "";
        if (omd.tags != null) {
            for (int i = 0; i < omd.tags.size(); i++) {
                String tag = (String)omd.tags.elementAt(i);
                tagstr = tagstr + tag;
                if (i != (omd.tags.size()-1)) {
                    tagstr = tagstr + ", ";
                }
            }
        }
        this.getContentPane().add(new JLabel(tagstr));

        this.getContentPane().add(new JLabel("Project:"));
        this.getContentPane().add(new JLabel(omd.project));

        this.getContentPane().add(new JLabel("Dataset:"));
        this.getContentPane().add(new JLabel(omd.dataset));

        this.getContentPane().add(new JLabel("Owner:"));
        this.getContentPane().add(new JLabel(omd.owner));

        this.getContentPane().add(new JLabel("Group:"));
        this.getContentPane().add(new JLabel(omd.group));

        this.getContentPane().add(new JLabel("Related files:"));
        int numrelfiles = 0;
        if (omd.relatedFiles != null) {
            numrelfiles = omd.relatedFiles.size();
        }
        this.getContentPane().add(new JLabel("" + numrelfiles));

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
