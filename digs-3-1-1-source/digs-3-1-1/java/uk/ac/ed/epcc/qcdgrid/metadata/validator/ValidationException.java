


package uk.ac.ed.epcc.qcdgrid.metadata.validator;



public class ValidationException extends Exception{
    public ValidationException( String message ){
	super( message );
    }

    public ValidationException( String message, Exception cause ){
	super( message, cause );
    }



}
