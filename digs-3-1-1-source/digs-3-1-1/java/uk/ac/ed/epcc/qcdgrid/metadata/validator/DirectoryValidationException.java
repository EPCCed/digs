
package uk.ac.ed.epcc.qcdgrid.metadata.validator;

import java.util.List;
import java.util.ArrayList;

public class DirectoryValidationException extends Exception {

    private List listOfInvalidFiles_ = new ArrayList();
    private int countOfInvalidFiles_ = 0;

    public DirectoryValidationException( String message ){
	super( message );
    }

    public void addFileToInvalidList( String fileName ){
	listOfInvalidFiles_.add( fileName );
    }
    
    public int getInvalidFileCount(){
	return listOfInvalidFiles_.size();
    }

    public List getInvalidFileList(){
	return listOfInvalidFiles_;
    }

    public String toString(){
	String ret = getMessage() + "\n";
	for(int i = 0; i<getInvalidFileCount(); i++){
	    ret += listOfInvalidFiles_.get(i) +"\n";
	}
	return ret;
    }

}
