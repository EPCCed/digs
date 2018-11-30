/*
 * Copyright (c) 2002 The University of Edinburgh. All rights reserved.
 *
 * Released under the OGSA-DAI Project Software Licence.
 * Please refer to licence.txt for full project software licence.
 */

package uk.ac.ed.epcc.qcdgrid.browser.GUI;

import java.io.File;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.*;
import java.util.List;
import java.net.URL;
import org.xml.sax.SAXException;

import uk.ac.ed.epcc.qcdgrid.browser.QueryHandler.*;
import uk.ac.ed.epcc.qcdgrid.browser.ResultHandlers.*;
import uk.ac.ed.epcc.qcdgrid.browser.Configuration.*;
import uk.ac.ed.epcc.qcdgrid.browser.DatabaseDrivers.*;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.ILDGQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.MultipleWebServiceQueryRunner;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.QCDgridQueryResults;
import uk.ac.ed.epcc.qcdgrid.metadata.catalogue.UserQueryRunner;

/**
 * Title: MainGUIFrame Description: The main window of the application
 * Copyright: Copyright (c) 2002 Company: The University of Edinburgh
 * 
 * @author Amy Krause
 * @version 1.0
 */

public class MainGUIFrame extends JFrame {

  /** GUI elements, mostly autogenerated by JBuilder */
  JPanel contentPane;

  JPanel scopePanel = new JPanel();

  JLabel statusBar = new JLabel();

  JButton clearScopeButton = new JButton();

  JMenuBar jMenuBar1 = new JMenuBar();

  JMenu jMenuFile = new JMenu();

  JMenuItem jMenuFileExit = new JMenuItem();

  JMenuItem jMenuFileImport = new JMenuItem();

  JMenuItem jMenuFileExport = new JMenuItem();

  JMenu jMenuHelp = new JMenu();

  JMenuItem jMenuHelpAbout = new JMenuItem();

  ImageIcon image1;

  ImageIcon image2;

  ImageIcon image3;

  BorderLayout borderLayout1 = new BorderLayout();

  JPanel contentPane1 = new JPanel();

  BorderLayout borderLayout2 = new BorderLayout();

  JButton jQueryNew = new JButton();

  JToolBar jToolBar1 = new JToolBar();

  JButton jRunQuery = new JButton();

  JButton jQueryCombine = new JButton();

  JButton jQueryDelete = new JButton();

  JButton jQueryCount = new JButton();

  JButton jQueryEdit = new JButton();

  JScrollPane jScrollPane2 = new JScrollPane();

  JLabel jLabel1 = new JLabel();

  JMenu jMenuQuery = new JMenu();

  JMenuItem jMenuQueryRun = new JMenuItem();

  JMenuItem jMenuQueryNew = new JMenuItem();

  JMenuItem jMenuQueryCombine = new JMenuItem();

  JMenuItem jMenuQueryDelete = new JMenuItem();

  JMenuItem jMenuQueryNewText = new JMenuItem();

  JMenuItem jMenuQueryCount = new JMenuItem();

  JMenuItem jMenuQueryEdit = new JMenuItem();

  JMenu jMenuOptions = new JMenu();

  JMenuItem jMenuSetLanguage = new JMenuItem();

  JMenuItem jMenuSetResultHandler = new JMenuItem();

  JMenuItem jMenuSettings = new JMenuItem();

  FeatureConfiguration featureConfiguration_;

  // own definitions
  /** the log displayer */
  TextDisplayer logWindow = new TextDisplayer(null, "Log Window");

  /**
   * a result handler, whose job it is to do something with the results of a
   * query. ResultHandler is an abstract base class from which different types
   * of handler can be derived
   */
  ResultHandler resultHandler;

  /** the query builder responsible for the queries */
  XPathQueryBuilder queryBuilder = null;

  /** the list of constructed queries */
  JList jQueryList;

  /** the browser's configuration */
  Configuration configuration = null;

  DatabaseDriver databaseDriver;

  static MainGUIFrame mainGUIFrame = null;

  boolean finished = false;

  public static MainGUIFrame getMainGUIFrame() {
    return mainGUIFrame;
  }

  /** Construct the frame */
  public MainGUIFrame() {

    try {
      featureConfiguration_ = new FeatureConfiguration();
    } catch (FeatureConfiguration.FeatureConfigurationException fce) {
      System.out.println("Using default browser modes.");
    }

    /* Allow us to catch window closing and exit properly */
    enableEvents(AWTEvent.WINDOW_EVENT_MASK);
    try {
      jbInit();
      gxdsInit();
      mainGUIFrame = this;
    } catch (SAXException e) {
      logWindow.append("Error parsing template document:" + e.getMessage());
      System.err.println("Error parsing template document:");
      System.err.println(e.getMessage());
      JOptionPane.showMessageDialog(null, "Error parsing template document.",
          "SAXException", JOptionPane.ERROR_MESSAGE);
      System.exit(0);
    } catch (NoQueryLanguageException e) {
      logWindow.append("Query language exception: " + e.getMessage());
      System.err.println("Query language exception:");
      System.err.println(e.getMessage());
      JOptionPane.showMessageDialog(this, e.getMessage(),
          "Query language exception", JOptionPane.ERROR_MESSAGE);
      System.exit(-1);
    } catch (Exception e) {
      logWindow.append("Error: " + e.getMessage());
      System.err.println(e.getMessage());
      JOptionPane.showMessageDialog(this, e.getMessage(),
          "Exception", JOptionPane.ERROR_MESSAGE);
      System.exit(-1);
    }
  }

  /** Component initialization - mostly autogenerated by JBuilder */
  private void jbInit() throws Exception {
    contentPane = (JPanel) this.getContentPane();
    contentPane.setLayout(borderLayout1);
    //this.setSize(new Dimension(464, 367));
    this.setSize(new Dimension(600, 400));
    this.setTitle("Browser");

    Point loc = getLocation();
    logWindow.setLocation(loc.x + 100, loc.y + 100);

    statusBar.setText("Scope: entire database");
    jMenuFile.setText("File");
    jMenuFileImport.setText("Import query library");
    jMenuFileImport.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuFileImport_actionPerformed(e);
      }
    });
    
    jMenuFileExport.setText("Export query library");
    jMenuFileExport.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuFileExport_actionPerformed(e);
      }
    });
    jMenuFileExit.setText("Close");
    jMenuFileExit.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuFileExit_actionPerformed(e);
      }
    });
    jMenuHelp.setText("Help");
    jMenuHelpAbout.setText("About");
    jMenuHelpAbout.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuHelpAbout_actionPerformed(e);
      }
    });
    contentPane1.setLayout(borderLayout2);
    jQueryNew.setText("New");
    jQueryNew.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryNew_actionPerformed(e);
      }
    });
    jRunQuery.setText("Run Query");
    jRunQuery.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jRunQuery_actionPerformed(e);
      }
    });
    jQueryCombine.setText("Combine");
    jQueryCombine.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryCombine_actionPerformed(e);

      }
    });
    jQueryDelete.setText("Delete");
    jQueryDelete.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryDelete_actionPerformed(e);
      }
    });
    jQueryCount.setText("Count");
    jQueryCount.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryCount_actionPerformed(e);
      }
    });
    jQueryEdit.setText("Edit");
    jQueryEdit.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryEdit_actionPerformed(e);
      }
    });
    clearScopeButton.setText("Clear");
    clearScopeButton.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        clearScopeButton_actionPerformed(e);
      }
    });

    jLabel1.setText("List of queries:");
    jMenuQuery.setText("Query");
    jMenuQueryRun.setText("Run");
    jMenuQueryRun.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuQueryRun_actionPerformed(e);
      }
    });
    jMenuQueryNew.setText("New...");
    jMenuQueryNew.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuQueryNew_actionPerformed(e);
      }
    });
    jMenuQueryCombine.setText("Combine");
    jMenuQueryCombine.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuQueryCombine_actionPerformed(e);
      }
    });
    jMenuQueryDelete.setText("Delete");
    jMenuQueryDelete.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuQueryDelete_actionPerformed(e);
      }
    });
    jMenuQueryNewText.setText("New text...");
    jMenuQueryNewText.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jMenuQueryNewText_actionPerformed(e);
      }
    });
    jMenuQueryCount.setText("Count");
    jMenuQueryCount.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryCount_actionPerformed(e);
      }
    });
    jMenuQueryEdit.setText("Edit");
    jMenuQueryEdit.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jQueryEdit_actionPerformed(e);
      }
    });
    jMenuOptions.setText("Options");
    jMenuSetLanguage.setText("Set query language");
    jMenuSetLanguage.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jSetLanguage_actionPerformed(e);
      }
    });
    jMenuSetResultHandler.setText("Set result handler");
    jMenuSetResultHandler
        .addActionListener(new java.awt.event.ActionListener() {
          public void actionPerformed(ActionEvent e) {
            jSetResultHandler_actionPerformed(e);
          }
        });
    jMenuSettings.setText("Settings...");
    jMenuSettings.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        jSettings_actionPerformed(e);
      }
    });

    scopePanel.setLayout(new BorderLayout());
    scopePanel.add(statusBar, BorderLayout.CENTER);
    scopePanel.add(clearScopeButton, BorderLayout.EAST);

    jMenuFile.add(jMenuFileImport);
    jMenuFile.add(jMenuFileExport);
    jMenuFile.add(jMenuFileExit);
    jMenuHelp.add(jMenuHelpAbout);
    jMenuBar1.add(jMenuFile);
    jMenuBar1.add(jMenuQuery);
    jMenuBar1.add(jMenuOptions);
    jMenuBar1.add(jMenuHelp);
    this.setJMenuBar(jMenuBar1);
    contentPane.add(scopePanel, BorderLayout.SOUTH);
    contentPane.add(jToolBar1, BorderLayout.NORTH);
    jToolBar1.add(jQueryNew, null);
    jToolBar1.add(jQueryCombine, null);
    jToolBar1.add(jQueryDelete, null);
    jToolBar1.add(jRunQuery, null);
    jToolBar1.add(jQueryCount, null);
    jToolBar1.add(jQueryEdit, null);
    contentPane.add(contentPane1, BorderLayout.CENTER);
    contentPane1.add(jScrollPane2, BorderLayout.CENTER);
    contentPane1.add(jLabel1, BorderLayout.NORTH);
    jMenuOptions.add(jMenuSetLanguage);
    jMenuOptions.add(jMenuSetResultHandler);
    jMenuOptions.add(jMenuSettings);
    jMenuQuery.add(jMenuQueryNew);
    jMenuQuery.add(jMenuQueryNewText);
    jMenuQuery.add(jMenuQueryCombine);
    jMenuQuery.add(jMenuQueryDelete);
    jMenuQuery.add(jMenuQueryRun);
    jMenuQuery.add(jMenuQueryCount);
    jMenuQuery.add(jMenuQueryEdit);
    // templateTree = new SAXTree();
  }

  /**
   * Performs non-GUI related initialisation - reads configuration, connects to
   * database service, etc.
   * 
   * @exception NoQueryLanguageException
   *              thrown if the database service doesn't support any of the same
   *              query languages we do
   * @exception Exception
   *              thrown if template document is empty
   */
  private void gxdsInit() throws NoQueryLanguageException, Exception {
    logWindow.setVisible(true);

    logWindow.append("Loading configuration...\n");

    /* Try to read user's config file */
    configuration = new Configuration();

    logWindow.append("done.\n");

    String dbu = configuration.getDatabaseUrl();
    if (dbu == null) {
      dbu = JOptionPane.showInputDialog(
          (Object) "Please enter the hostname and port number of the central database",
          (Object) "localhost:8080");

      configuration.setDatabaseUrl(dbu);
    }

    logWindow.append("Connecting to database service...\n");
    /* initialise connection to the grid database service */
    databaseDriver = new QCDgridExistDatabase();
    
    FeatureConfiguration fc = new FeatureConfiguration();    
    databaseDriver.init(dbu, fc.getXMLDBUserName(), fc.getXMLDBPassword());
    logWindow.append("done.\n");

    /* retrieve the template document from the database service */
    /*
     * logWindow.append("Retrieving schema document...\n"); String
     * templateString = databaseDriver.getSchema(); if (templateString == null)
     * throw(new Exception("Empty schema")); logWindow.append("done.\n");
     */

    /* retrieve the query notation types */
    logWindow.append("Retrieving supported query notations...\n");
    Vector queryNotations = databaseDriver.getQueryNotations();
    logWindow.append("done.\n");
    String queryLanguage = selectQueryLanguage(queryNotations);
    /* at the moment we only support XPath... */

    /*
     * let the user choose what to do with query results, if it's not in their
     * config file
     */
    selectResultHandler();

    logWindow.append("Building tree from schema...\n");
    // initialise the query builder with the template document
    // this builds the SAXTree from the template

    QueryTypeManager qtm = new QueryTypeManager();

    queryBuilder = new XPathQueryBuilder(this, qtm);
    logWindow.append("done.\n");

    /* get any saved queries into the query builder */
    configuration.getQueries((XPathQueryBuilder) queryBuilder);

    /* initialise the query list */
    jQueryList = new JList(queryBuilder.getQueryList());
    jScrollPane2.getViewport().add(jQueryList, null);
  }

  /**
   * Determines the query language to use, using the language specified in the
   * configuration if there is one, otherwise prompts the user to choose one
   * from a list
   * 
   * @param q
   *          a vector of supported query languages from the server
   * @return a string representing the chosen language
   * @exception NoQueryLanguageException
   *              thrown if the server doesn't support any suitable languages
   */
  private String selectQueryLanguage(Vector q) throws NoQueryLanguageException {

    /* see if it's stored in the configuration */
    String ql = configuration.getQueryLanguage();

    /* check configured language is supported */
    boolean supported = false;

    if (ql != null) {
      for (int i = 0; i < q.size(); i++) {
        /* This needs to be amended if new query notations are ever supported */
        if ((q.contains("http://www.w3.org/TR/1999/REC-xpath-19991116"))
            && (ql.equals("XPath"))) {
          supported = true;
        }
      }
    }

    /* if not, ask the user now */
    if ((ql == null) || (!supported)) {
      try {
        QueryNotationSelector dlg = new QueryNotationSelector(q);
        Dimension dlgSize = dlg.getPreferredSize();
        Dimension frmSize = getSize();
        Point loc = getLocation();
        dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
            (frmSize.height - dlgSize.height) / 2 + loc.y);
        dlg.setModal(true);
        dlg.show();
        ql = dlg.getSelectedLanguage();
        configuration.setQueryLanguage(ql);
      } catch (Exception e) {
        throw (new NoQueryLanguageException(
            "Client and Server do not support common query languages."));
      }
    }
    return ql;
  }

  /**
   * Determines which result handler plugin to use. First tries to read a class
   * name from the configuration, if that fails allows the user to select from a
   * list.
   */
  private void selectResultHandler() {
    String resultHandlerClass;
    try {
      /* see if result handler is in configuration */
      resultHandlerClass = configuration.getResultHandler();

      /* if not, get user to choose one now */
      if (resultHandlerClass == null) {
        ResultHandlerSelector dlg = new ResultHandlerSelector();
        Dimension dlgSize = dlg.getPreferredSize();
        Dimension frmSize = getSize();
        Point loc = getLocation();
        dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
            (frmSize.height - dlgSize.height) / 2 + loc.y);
        dlg.setModal(true);
        dlg.show();
        resultHandler = dlg.getSelectedHandler();

        /* Set name in configuration so that it "remembers" */
        resultHandlerClass = resultHandler.getClass().getName();
        configuration.setResultHandler(resultHandlerClass);
      } else {
        /* Instantiate result handler */
        Class c = Class.forName(resultHandlerClass);
        resultHandler = (ResultHandler) c.newInstance();
      }
    } catch (Exception e) {
      System.err.println("Exception " + e
          + " occurred while selecting result handler - using default");

      /*
       * Selected plugin caused an exception, so use the default one, a simple
       * text window
       */
      resultHandler = new SimpleDisplayResultHandler();
      resultHandlerClass = "browser.ResultHandlers.SimpleDisplayResultHandler";
      configuration.setResultHandler(resultHandlerClass);
    }
  }

  public void exit() {
      if ((configuration != null) && (queryBuilder != null)) {
	  configuration.setQueries(queryBuilder);
	  configuration.writeConfigFile();
      }
      // System.exit(0);
      logWindow.dispose();
      dispose();
      finished = true;
  }

  /** File | Exit action performed */
  public void jMenuFileExit_actionPerformed(ActionEvent e) {
      exit();
  }

  public void jMenuFileImport_actionPerformed(ActionEvent e) {
    JFileChooser chooser = new JFileChooser();

    chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
    chooser.setDialogTitle("Import query library...");
    chooser.setFileFilter(new javax.swing.filechooser.FileFilter() {
      public boolean accept(File f) {
        if (f.getPath().endsWith(".xml")) {
          return true;
        }
        // Without this, it's impossible to browser the directory structure
        if (f.isDirectory()) {
          return true;
        }
        return false;
      }

      public String getDescription() {
        return "XML Documents";
      }
    });
    if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
      queryBuilder.importLibrary(chooser.getSelectedFile().getPath());
    }
  }

  public void jMenuFileExport_actionPerformed(ActionEvent e) {

    if (jQueryList.isSelectionEmpty()) {
      JOptionPane.showMessageDialog(this, "You must select some queries first",
          "Error", JOptionPane.ERROR_MESSAGE); 
      return;
    }

    JFileChooser chooser = new JFileChooser();

    chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
    chooser.setDialogTitle("Export query library as...");
    chooser.setFileFilter(new javax.swing.filechooser.FileFilter() {
      public boolean accept(File f) {
        if (f.getPath().endsWith(".xml")) {
          return true;
        }
        // Without this, it's impossible to browser the directory structure
        if (f.isDirectory()) {
          return true;
        }
        return false;
      }

      public String getDescription() {
        return "XML Documents";
      }
    });
    if (chooser.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
      queryBuilder.exportLibrary(chooser.getSelectedFile().getPath(),
          jQueryList);
    }
  }

  /** Help | About action performed */
  public void jMenuHelpAbout_actionPerformed(ActionEvent e) {
    MainGUIFrame_AboutBox dlg = new MainGUIFrame_AboutBox(this);
    Dimension dlgSize = dlg.getPreferredSize();
    Dimension frmSize = getSize();
    Point loc = getLocation();
    dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    dlg.setModal(true);
    dlg.show();
  }

  /** Overridden so we can exit when window is closed */
  protected void processWindowEvent(WindowEvent e) {
    super.processWindowEvent(e);
    if (e.getID() == WindowEvent.WINDOW_CLOSING) {
      jMenuFileExit_actionPerformed(null);
    }
  }

  /** toolbar button "Combine" */
  void jQueryCombine_actionPerformed(ActionEvent e) {
    queryBuilder.combineQueries(jQueryList);
  }

  /** toolbar button "New" */
  void jQueryNew_actionPerformed(ActionEvent e) {
    queryBuilder.newQuery(configuration.getNamespace());
  }

  /** toolbar button "Delete" */
  void jQueryDelete_actionPerformed(ActionEvent e) {
    queryBuilder.deleteQuery(jQueryList);
  }

  /**
   * Calls queryBuilder.runQuery to send a query to the XML database service and
   * sends the results to the selected result handler plugin for further
   * processing
   */
  void runMyQuery(ActionEvent e) {
    try {
      logWindow.append("Sending query...\n");
      // Date queryTime = Calendar.getInstance().getTime();
      UserQueryRunner queryRunner;
      if (featureConfiguration_.getBrowserMode().equals(FeatureConfiguration.UKQCD_MODE)) {
        //System.out.println("UKQCD browser mode, using UKQCD query runner");
        queryRunner = (UserQueryRunner) databaseDriver;
      } else {
        //System.out.println("ILDG browser mode, using ILDG query runner");
        ServicesFile sf = new ServicesFile(featureConfiguration_
            .getServicesFileLocation());
        List services = sf.getMetadataCatalogueServiceList();
        MultipleWebServiceQueryRunner qr = new MultipleWebServiceQueryRunner(
            services);
        if(services==null || services.size()==0){
          logWindow.append("Warning: no services file found, or zero services specified");
        }
        queryRunner = (UserQueryRunner) qr;
      }
      QCDgridQueryResults result = queryBuilder.runUserQuery(jQueryList,
          queryRunner, configuration.getNamespace());
      logWindow.append("received results.\n");
      

      // Handle any errors that occurred if it was running under ILDG mode
      String browserMode = null;
      try{
        FeatureConfiguration fc = new FeatureConfiguration();
        browserMode = fc.getBrowserMode();
      }
      catch(FeatureConfiguration.FeatureConfigurationException fce){
        browserMode = FeatureConfiguration.UKQCD_MODE;
      }
      if(browserMode.equals(FeatureConfiguration.ILDG_MODE)){
        MultipleWebServiceQueryRunner mqr = (MultipleWebServiceQueryRunner) queryRunner;
        if(mqr.hasErrors()){
          System.out.println("One or more possibly non-fatal errors occurred when"
              + " running this query:");
          List errors = mqr.getErrors();
          for(int i=0; i<errors.size(); i++){
            System.out.println((String)errors.get(i
                ));
          }
        }
      }
      
      resultHandler.handleResults(result);
    }
    /*
     * catch (InvalidQueryNotationError er) {
     * logWindow.append("InvalidQueryNotationError: " + er.getErrorMsg()); }
     * catch (InvalidQueryError er) { logWindow.append("InvalidQueryError: " +
     * er.getErrorMsg()); }
     */
    catch (java.rmi.RemoteException exc) {
      logWindow.append("java.rmi.RemoteException: " + exc.getMessage());
    } catch (Exception exc) {
      logWindow.append("Error: " + exc.getMessage());
    }
  }

  /**
   * Counts how many documents are returned by the selected query. Prints the
   * result to System.out
   */
  void runMyCount(ActionEvent e) {
    try {
      logWindow.append("Sending query...\n");
      // Date queryTime = Calendar.getInstance().getTime();
      CountResultHandler rh = new CountResultHandler();
      QCDgridQueryResults result = queryBuilder.runQuery(jQueryList,
          databaseDriver, configuration.getNamespace());
      logWindow.append("received results.\n");

      rh.handleResults(result);
      System.out.println("Count result: " + rh.getCount());
    }
    /*
     * catch (InvalidQueryNotationError er) {
     * logWindow.append("InvalidQueryNotationError: " + er.getErrorMsg()); }
     * catch (InvalidQueryError er) { logWindow.append("InvalidQueryError: " +
     * er.getErrorMsg()); }
     */
    catch (java.rmi.RemoteException exc) {
      logWindow.append("java.rmi.RemoteException: " + exc.getMessage());
    } catch (Exception exc) {
      logWindow.append("Error: " + exc.getMessage());
    }
  }

  /** toolbar button "Run" */
  void jRunQuery_actionPerformed(ActionEvent e) {
    runMyQuery(e);
  }

  /** Query | New... */
  void jMenuQueryNew_actionPerformed(ActionEvent e) {
    queryBuilder.newQuery(configuration.getNamespace());
  }

  /** Query | Combine */
  void jMenuQueryCombine_actionPerformed(ActionEvent e) {
    queryBuilder.combineQueries(jQueryList);
  }

  /** Query | Delete */
  void jMenuQueryDelete_actionPerformed(ActionEvent e) {
    queryBuilder.deleteQuery(jQueryList);
  }

  /** Query | Run */
  void jMenuQueryRun_actionPerformed(ActionEvent e) {
    runMyQuery(e);
  }

  /** Query | Count */
  void jQueryCount_actionPerformed(ActionEvent e) {
    runMyCount(e);
  }

  /** Query | New Text... */
  void jMenuQueryNewText_actionPerformed(ActionEvent e) {
    queryBuilder.newTextQuery();
  }

  /** Query | Edit */
  void jQueryEdit_actionPerformed(ActionEvent e) {
    queryBuilder.editQuery(jQueryList, configuration.getNamespace());
  }

  /** Options | Set Language */
  void jSetLanguage_actionPerformed(ActionEvent e) {
    try {
      /* retrieve the query notation types */
      // QueryNotationTypesRetriever q = new
      // QueryNotationTypesRetriever(gridService);
      logWindow.append("Retrieving supported query notations...\n");
      Vector queryNotations = databaseDriver.getQueryNotations();
      logWindow.append("done.\n");
      QueryNotationSelector dlg = new QueryNotationSelector(queryNotations);
      Dimension dlgSize = dlg.getPreferredSize();
      Dimension frmSize = getSize();
      Point loc = getLocation();
      dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
          (frmSize.height - dlgSize.height) / 2 + loc.y);
      dlg.setModal(true);
      dlg.show();
      String ql = dlg.getSelectedLanguage();
      configuration.setQueryLanguage(ql);
    } catch (Exception ex) {
      System.err
          .println("Client and Server do not support common query languages.");
    }
  }

  /** Options | Set Result Handler */
  void jSetResultHandler_actionPerformed(ActionEvent e) {
    try {
      ResultHandlerSelector dlg = new ResultHandlerSelector();
      Dimension dlgSize = dlg.getPreferredSize();
      Dimension frmSize = getSize();
      Point loc = getLocation();
      dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
          (frmSize.height - dlgSize.height) / 2 + loc.y);
      dlg.setModal(true);
      dlg.show();
      ResultHandler rh = dlg.getSelectedHandler();
      if(rh != null){
	  resultHandler = rh;

	  /* Set name in configuration so that it "remembers" */
	  String resultHandlerClass = resultHandler.getClass().getName();
	  configuration.setResultHandler(resultHandlerClass);
      }
      /* if null, don't change it */
    } catch (Exception ex) {
      System.err
          .println("Error setting result handler, using default instead: " + e);
      ex.printStackTrace();
      /* Use the default one, a simple text window */
      resultHandler = new SimpleDisplayResultHandler();
      String resultHandlerClass = "browser.ResultHandlers.SimpleDisplayResultHandler";
      configuration.setResultHandler(resultHandlerClass);
    }
  }

  /* Options | Settings */
  void jSettings_actionPerformed(ActionEvent e) {
    SettingsDialogue dlg = new SettingsDialogue(configuration, this);
    Dimension dlgSize = dlg.getPreferredSize();
    Dimension frmSize = getSize();
    Point loc = getLocation();
    dlg.setLocation((frmSize.width - dlgSize.width) / 2 + loc.x,
        (frmSize.height - dlgSize.height) / 2 + loc.y);
    dlg.show();
  }

  void clearScopeButton_actionPerformed(ActionEvent e) {
    statusBar.setText("Scope: entire database");
    try {
	queryBuilder.setScope("");
    } catch (Exception ex) {
      logWindow.append("Exception setting database scope: " + ex);
    }
  }

  void updateScopeCaption(String newScope) {
    statusBar.setText("Scope: " + newScope);
  }

  public void setScope(String scope) {
      queryBuilder.setScope(scope);
      updateScopeCaption(scope);
  }

  public boolean isFinished() {
      return finished;
  }
}

/**
 * A NoQueryLanguageException is thrown if the browser and the XML database
 * service do not support common query languages
 */
class NoQueryLanguageException extends Exception {
  public NoQueryLanguageException(String msg) {
    super(msg);
  }
}