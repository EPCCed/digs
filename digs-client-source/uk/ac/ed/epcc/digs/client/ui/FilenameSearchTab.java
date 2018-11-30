package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class FilenameSearchTab extends SearchTab {

    private String searchString;
    
    private JLabel label;
    private JTextField field;

    public FilenameSearchTab() {
        searchString = null;

        label = new JLabel("Search for filenames containing:");
        field = new JTextField(20);

        this.setLayout(new FlowLayout());
        this.add(label);
        this.add(field);
    }

    public String getTabName() {
        return "Search by filename";
    }

    public String getSearchString() {
        return field.getText();
    }
}
