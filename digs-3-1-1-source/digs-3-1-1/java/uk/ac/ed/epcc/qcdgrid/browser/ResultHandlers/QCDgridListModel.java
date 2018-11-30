/*
 * Copyright (c) 2003 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */
package uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers;

import javax.swing.*;
import java.util.Vector;

/**
 * Title: QCDgridListModel Description: This class provides the list data to the
 * list of documents found in the QCDgrid result handler GUI Copyright:
 * Copyright (c) 2003 Company: The University of Edinburgh
 * 
 * @author James Perry
 * @version 1.0
 */
public class QCDgridListModel extends AbstractListModel {

  /** Holds the current contents of the list */
  private Vector content;

  /** Constructor: creates an empty list model */
  public QCDgridListModel() {
    super();

    content = new Vector();
  }

  /**
   * Returns an element of the list
   * 
   * @param index
   *          which element to get
   * @return the element requested
   */
  public Object getElementAt(int index) {
    if (index >= content.size()) {
      return null;
    } else {
      return content.elementAt(index);
    }
  }

  /**
   * @return the number of items currently in the list
   */
  public int getSize() {
    return content.size();
  }

  /**
   * Adds a new item to the end of the list
   * 
   * @param item
   *          the text of the item to add
   */
  public void addItem(Object item) {
    content.add(item);
    fireIntervalAdded(this, content.size() - 1, content.size() - 1);
  }
}
