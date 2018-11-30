package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import java.util.Vector;

import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryTypeManager;
import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.QueryType;

public class QueryTypeSelector extends JDialog {

  /** widgets */
  JPanel jPanel2 = new JPanel();

  JButton jOK = new JButton();

  JScrollPane jScrollPane1 = new JScrollPane();

  Vector queryTypeList = new Vector();

  JList jQueryTypeList;

  JLabel jLabel1 = new JLabel();

  QueryTypeManager queryTypeManager;

  QueryType selectedType;

  public QueryTypeSelector(QueryTypeManager qtm) {

    /* Build a list of the descriptions */
    Vector qts = qtm.getQueryTypeList();
    for (int i = 0; i < qts.size(); i++) {
      QueryType qt = (QueryType) qts.elementAt(i);
      queryTypeList.add(qt.getDescription());
    }

    jQueryTypeList = new JList(queryTypeList);

    jOK.setText("OK");
    jOK.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jOK_actionPerformed(e);
      }
    });
    jLabel1.setText("Please choose a query type:");
    jQueryTypeList.setSelectedIndex(0);
    jScrollPane1.setPreferredSize(new Dimension(200, 200));
    this.getContentPane().add(jPanel2, BorderLayout.SOUTH);
    jPanel2.add(jOK, null);
    this.getContentPane().add(jScrollPane1, BorderLayout.CENTER);
    jScrollPane1.getViewport().add(jQueryTypeList, null);
    this.getContentPane().add(jLabel1, BorderLayout.NORTH);

    queryTypeManager = qtm;
    selectedType = null;

    pack();
  }

  /** OK button action event handler */
  void jOK_actionPerformed(ActionEvent e) {
    // selectedLanguage = (String)jQueryNotationList.getSelectedValue();
    int i = jQueryTypeList.getSelectedIndex();

    selectedType = (QueryType) queryTypeManager.getQueryTypeList().elementAt(i);

    dispose();
  }

  public QueryType getSelectedType() {
    return selectedType;
  }
}
