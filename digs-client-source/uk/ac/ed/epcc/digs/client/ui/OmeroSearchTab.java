package uk.ac.ed.epcc.digs.client.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;

import java.util.Vector;

public class OmeroSearchTab extends SearchTab {

    private String searchString;
    
    private JList userList;
    private JList groupList;
    private JList projectList;
    private JList datasetList;

    private JList tagList;

    private JTextField nameField;
    private JTextField descriptionField;
    private JTextField commentField;
    private JTextField linkField;

    public OmeroSearchTab() {
        try {
            OMEROInterface omero = OMEROInterface.getInstance();

            Vector users = omero.getUsers();
            Vector groups = omero.getGroups();
            Vector projects = omero.getProjects();
            Vector datasets = omero.getDatasets();
            Vector tags = omero.getTags();

            searchString = null;

            Border blackline = BorderFactory.createLineBorder(Color.black);

            JPanel leftPanel = new JPanel();
            JPanel midPanel = new JPanel();
            JPanel rightPanel = new JPanel();
            
            JPanel topMidPanel = new JPanel();
            JPanel bottomMidPanel = new JPanel();
            JPanel topRightPanel = new JPanel();
            JPanel bottomRightPanel = new JPanel();

            JPanel topLeftPanel = new JPanel();
            JPanel middleLeftPanel = new JPanel();
            JPanel bottomLeftPanel = new JPanel();

            JPanel namePanel = new JPanel();
            JPanel descriptionPanel = new JPanel();
            JPanel commentPanel = new JPanel();
            JPanel linkPanel = new JPanel();

            namePanel.setLayout(new BorderLayout());
            descriptionPanel.setLayout(new BorderLayout());
            commentPanel.setLayout(new BorderLayout());
            linkPanel.setLayout(new BorderLayout());

            nameField = new JTextField();
            namePanel.add(new JLabel("Name contains:"), BorderLayout.NORTH);
            namePanel.add(nameField, BorderLayout.CENTER);

            descriptionField = new JTextField();
            descriptionPanel.add(new JLabel("Description contains:"), BorderLayout.NORTH);
            descriptionPanel.add(descriptionField, BorderLayout.CENTER);

            commentField = new JTextField();
            commentPanel.add(new JLabel("Comments contain:"), BorderLayout.NORTH);
            commentPanel.add(commentField, BorderLayout.CENTER);

            linkField = new JTextField();
            linkPanel.add(new JLabel("Links contain:"), BorderLayout.NORTH);
            linkPanel.add(linkField, BorderLayout.CENTER);

            topLeftPanel.setLayout(new GridLayout(2, 1));
            middleLeftPanel.setLayout(new GridLayout(2, 1));
            bottomLeftPanel.setLayout(new BorderLayout());
            
            topLeftPanel.add(namePanel);
            topLeftPanel.add(descriptionPanel);
            middleLeftPanel.add(commentPanel);
            middleLeftPanel.add(linkPanel);

            tagList = new JList(tags);
            tagList.setBorder(blackline);
            bottomLeftPanel.add(new JLabel("With tags:"), BorderLayout.NORTH);
            bottomLeftPanel.add(tagList, BorderLayout.CENTER);

            userList = new JList(users);
            userList.setBorder(blackline);
            userList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            topMidPanel.setLayout(new BorderLayout());
            topMidPanel.add(new JLabel("Owned by:"), BorderLayout.NORTH);
            topMidPanel.add(userList, BorderLayout.CENTER);

            projectList = new JList(projects);
            projectList.setBorder(blackline);
            projectList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            bottomMidPanel.setLayout(new BorderLayout());
            bottomMidPanel.add(new JLabel("In project:"), BorderLayout.NORTH);
            bottomMidPanel.add(projectList, BorderLayout.CENTER);

            groupList = new JList(groups);
            groupList.setBorder(blackline);
            groupList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            topRightPanel.setLayout(new BorderLayout());
            topRightPanel.add(new JLabel("In group:"), BorderLayout.NORTH);
            topRightPanel.add(groupList, BorderLayout.CENTER);

            datasetList = new JList(datasets);
            datasetList.setBorder(blackline);
            datasetList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            bottomRightPanel.setLayout(new BorderLayout());
            bottomRightPanel.add(new JLabel("In dataset:"), BorderLayout.NORTH);
            bottomRightPanel.add(datasetList, BorderLayout.CENTER);
            
            midPanel.setLayout(new GridLayout(2, 1));
            midPanel.add(topMidPanel);
            midPanel.add(bottomMidPanel);
            
            rightPanel.setLayout(new GridLayout(2, 1));
            rightPanel.add(topRightPanel);
            rightPanel.add(bottomRightPanel);

            leftPanel.setLayout(new GridLayout(3, 1));
            leftPanel.add(topLeftPanel);
            leftPanel.add(middleLeftPanel);
            leftPanel.add(bottomLeftPanel);
            
            this.setLayout(new GridLayout(1, 3));
            this.add(leftPanel);
            this.add(midPanel);
            this.add(rightPanel);
        }
        catch (Exception ex) {
            System.err.println("OMERO error: " + ex);
        }
    }

    public String getTabName() {
        return "Search by OMERO metadata";
    }

    public String getSearchString() {
        /*
         * Build an OMERO metadata structure from the UI input
         */
        OMEROMetadata omd = new OMEROMetadata();

        String name = nameField.getText();
        if ((name != null) && (!name.equals(""))) {
            omd.filename = name;
        }
        String desc = descriptionField.getText();
        if ((desc != null) && (!name.equals(""))) {
            omd.description = desc;
        }
        String comment = commentField.getText();
        if ((comment != null) && (!comment.equals(""))) {
            omd.comments = new Vector();
            omd.comments.add(comment);
        }
        String link = linkField.getText();
        if ((link != null) && (!link.equals(""))) {
            omd.links = new Vector();
            omd.links.add(link);
        }
        Object[] seltags = tagList.getSelectedValues();
        if ((seltags != null) && (seltags.length > 0)) {
            omd.tags = new Vector();
            for (int i = 0; i < seltags.length; i++) {
                omd.tags.add(seltags[i]);
            }
        }
        Object owner = userList.getSelectedValue();
        if (owner != null) {
            omd.owner = (String)owner;
        }
        Object group = groupList.getSelectedValue();
        if (group != null) {
            omd.group = (String)group;
        }
        Object project = projectList.getSelectedValue();
        if (project != null) {
            omd.project = (String)project;
        }
        Object dataset = datasetList.getSelectedValue();
        if (dataset != null) {
            omd.dataset = (String)dataset;
        }

        try {
            OMEROInterface omero = OMEROInterface.getInstance();
            
            Vector results = omero.metadataSearch(omd);
            if ((results == null) || (results.size() == 0)) {
                searchString = "";
            }
            else {
                searchString = "list:";
                for (int i = 0; i < results.size(); i++) {
                    searchString = searchString + (String)results.elementAt(i);
                    if (i != (results.size() - 1)) {
                        searchString = searchString + ",";
                    }
                }
            }
        }
        catch (Exception ex) {
            System.err.println("Exception while searching OMERO: " + ex);
        }
        
        return searchString;
    }
}
