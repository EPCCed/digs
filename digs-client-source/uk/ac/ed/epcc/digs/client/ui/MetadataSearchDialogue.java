package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import java.util.Vector;

public class MetadataSearchDialogue extends JDialog implements ActionListener {

    private JPanel buttonPanel;

    private JButton okButton;
    private JButton cancelButton;

    private JTabbedPane tabs;

    private String searchString;

    private Vector searchTabs;

    private void addTab(SearchTab st) {
        searchTabs.add(st);
        tabs.add(st.getTabName(), st);
    }

    public MetadataSearchDialogue(Frame parent) {
        super(parent, "Search Metadata", true);
        
        searchString = null;
        searchTabs = new Vector();

        okButton = new JButton("OK");
        okButton.addActionListener(this);
        cancelButton = new JButton("Cancel");
        cancelButton.addActionListener(this);
        buttonPanel = new JPanel();
        buttonPanel.setLayout(new FlowLayout());
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);

        tabs = new JTabbedPane();

        addTab(new FilenameSearchTab());
        addTab(new OmeroSearchTab());

        this.getContentPane().setLayout(new BorderLayout());
        this.getContentPane().add(tabs, BorderLayout.CENTER);
        this.getContentPane().add(buttonPanel, BorderLayout.SOUTH);

        //this.setPreferredSize(new Dimension(400, 300));

        pack();
        setLocationRelativeTo(null);
        setVisible(true);        
    }

    public String getSearchString() {
        return searchString;
    }

    public void actionPerformed(ActionEvent e) {
        Object source = e.getSource();
        if (source == okButton) {
            SearchTab st = (SearchTab) tabs.getSelectedComponent();
            searchString = st.getSearchString();
            dispose();
        }
        else if (source == cancelButton) {
            dispose();
        }
    }
}
