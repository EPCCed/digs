/*
 * Contains all the code for talking to an OMERO server
 */
package uk.ac.ed.epcc.digs.client.ui;

import uk.ac.ed.epcc.digs.DigsException;
import java.util.Vector;
import java.util.List;
import java.util.Set;
import java.util.Map;
import java.util.Iterator;
import java.util.Collection;
import java.util.HashSet;

public class OMEROInterface {

    private static OMEROInterface instance = null;

    private String hostname;

    private ome.system.ServiceFactory serviceFactory;
    private ome.api.IQuery queryService;
    private ome.api.IPojos containerService;
    private ome.api.IAdmin adminService;

    private boolean initialised;

    private OMEROInterface() {
        hostname = null;
        serviceFactory = null;
        initialised = false;
        queryService = null;
        containerService = null;
        adminService = null;
    }

    private void startup() throws Exception {
        // check if connected already
        if (initialised) {
            return;
        }

        // must have hostname set
        if (hostname == null) {
            throw new DigsException("OMERO server hostname not set");
        }
        
        // get username and password
        OMEROLoginDialogue ld = new OMEROLoginDialogue(null);
        String username = ld.getUsername();
        if (username == null) {
            throw new DigsException("User cancelled login");
        }
        char[] pwd = ld.getPassword();
        String password = new String(pwd);

        // try to connect
        ome.system.Login login = new ome.system.Login(username, password);
        ome.system.Server server = new ome.system.Server(hostname, 1099);
        serviceFactory = new ome.system.ServiceFactory(server, login);
        
        // get services
        queryService = serviceFactory.getQueryService();
        containerService = serviceFactory.getPojosService(); // FIXME: container service in new OMERO versions
        adminService = serviceFactory.getAdminService();

        initialised = true;
    }

    // gets a list of all the files in OMERO
    public Vector listFiles() throws Exception {
        startup();

        Vector result = new Vector();
        List<ome.model.core.Image> list = queryService.findAll(ome.model.core.Image.class, null);
        if (list != null) {
            for (int i = 0; i < list.size(); i++) {
                ome.model.core.Image img = list.get(i);
                result.add(img.getName());
            }
        }

        return result;
    }

    // gets the content of a file
    public byte[] getFile(String name) throws Exception {
        startup();

        ome.model.core.OriginalFile of = queryService.findByString(ome.model.core.OriginalFile.class, "name", name);
        if (of == null) return null;
        return getFile(of.getId());
    }

    // gets the content of a file
    public byte[] getFile(long id) throws Exception {
        startup();

        ome.model.core.OriginalFile of = queryService.get(ome.model.core.OriginalFile.class, id);

        ome.api.RawFileStore rfs = serviceFactory.createRawFileStore();
        rfs.setFileId(id);
        
        byte[] contents = rfs.read(0L, of.getSize().intValue());

        return contents;
    }

    /*
     * Search for images based on a partially filled in OMEROMetadata structure
     */
    public Vector metadataSearch(OMEROMetadata search) throws Exception {
        startup();
        Vector results = new Vector();
        
        /*
         * Will be able to search on:
         *  - name and description (contains)
         *  - tag values (equality)
         *  - comment and url (contains)
         *  - owner and group (equality)
         *  - project and dataset (equality)
         */

        /*
         * If a project or dataset is specified, first find those and get lists of images in them, so we
         * only have to do that once
         */
        HashSet<Long> dsImages = null;
        HashSet<Long> prjImages = null;
        if (search.project != null) {
            ome.model.containers.Project prj = queryService.findByString(ome.model.containers.Project.class, "name", search.project);
            prjImages = new HashSet<Long>();
            Set<Long> idset = new HashSet<Long>();
            idset.add(new Long(prj.getId()));
            Set<ome.model.core.Image> imgset = containerService.getImages(ome.model.containers.Project.class, idset, null);
            Iterator iter = imgset.iterator();
            while (iter.hasNext()) {
                ome.model.core.Image img = (ome.model.core.Image)iter.next();
                prjImages.add(img.getId());
            }
        }
        if (search.dataset != null) {
            ome.model.containers.Dataset ds = queryService.findByString(ome.model.containers.Dataset.class, "name", search.dataset);
            dsImages = new HashSet<Long>();
            Set<Long> idset = new HashSet<Long>();
            idset.add(new Long(ds.getId()));
            Set<ome.model.core.Image> imgset = containerService.getImages(ome.model.containers.Dataset.class, idset, null);
            Iterator iter = imgset.iterator();
            while (iter.hasNext()) {
                ome.model.core.Image img = (ome.model.core.Image)iter.next();
                dsImages.add(img.getId());
            }
        }
        
        HashSet<Long> urlMatches = null;
        HashSet<Long> tagMatches = null;
        HashSet<Long> commentMatches = null;
        String searchUrl = null;
        String searchComment = null;

        /*
         * If tags, comments or URLs are specified, narrow down the search based on those
         */
        if (((search.links != null) && (search.links.size() > 0)) ||
            ((search.comments != null) && (search.comments.size() > 0)) ||
            ((search.tags != null) && (search.tags.size() > 0))) {
            
            if ((search.links != null) && (search.links.size() > 0)) {
                urlMatches = new HashSet<Long>();
                searchUrl = (String)search.links.elementAt(0);
            }
            if ((search.comments != null) && (search.comments.size() > 0)) {
                commentMatches = new HashSet<Long>();
                searchComment = (String)search.comments.elementAt(0);
            }
            if ((search.tags != null) && (search.tags.size() > 0)) {
                tagMatches = new HashSet<Long>();
            }

            Set<Long> idset = new HashSet<Long>();
            List<ome.model.core.Image> imglist = queryService.findAll(ome.model.core.Image.class, null);
            for (int i = 0; i < imglist.size(); i++) {
                ome.model.core.Image img = imglist.get(i);
                idset.add(new Long(img.getId()));
            }

            Map annotations = containerService.findAnnotations(ome.model.core.Image.class, idset, null, null);
            if (annotations != null) {
                Set keys = annotations.keySet();
                Iterator iter = keys.iterator();
                while (iter.hasNext()) {
                    // image ID we're currently looking at
                    Long currid = (Long)iter.next();
                    Set annots = (Set)annotations.get(currid);
                    if (annots != null) {
                        int matchedComment = 0;
                        int matchedUrl = 0;
                        int matchedTag = 0;

                        Iterator iter2 = annots.iterator();
                        while (iter2.hasNext()) {
                            Object annobj = iter2.next();
                            if (annobj instanceof ome.model.annotations.UrlAnnotation) {
                                pojos.URLAnnotationData uad = new pojos.URLAnnotationData((ome.model.annotations.UrlAnnotation) annobj);
                                if ((searchUrl != null) && (uad.getURL().contains(searchUrl))) {
                                    matchedUrl++;
                                }
                            }
                            else if (annobj instanceof ome.model.annotations.TagAnnotation) {
                                pojos.TagAnnotationData tad = new pojos.TagAnnotationData((ome.model.annotations.TagAnnotation) annobj);
                                if (tagMatches != null) {
                                    for (int j = 0; j < search.tags.size(); j++) {
                                        if (tad.getTagValue().equals((String)search.tags.elementAt(j))) {
                                            matchedTag++;
                                        }
                                    }
                                }
                            }
                            else if (annobj instanceof ome.model.annotations.TextAnnotation) {
                                pojos.TextualAnnotationData tad = new pojos.TextualAnnotationData((ome.model.annotations.TextAnnotation) annobj);
                                if ((searchComment != null) && (tad.getText().contains(searchComment))) {
                                    matchedComment++;
                                }
                            }
                        }

                        // deal with matches for this image
                        if (matchedComment > 0) {
                            commentMatches.add(currid);
                        }
                        if (matchedUrl > 0) {
                            urlMatches.add(currid);
                        }
                        if (search.tags != null) {
                            if (matchedTag >= search.tags.size()) {
                                tagMatches.add(currid);
                            }
                        }
                    }
                }
            }
        }


        /*
         * Now iterate over images and test them
         */
        List<ome.model.core.Image> list = queryService.findAll(ome.model.core.Image.class, null);
        if (list != null) {

            for (int i = 0; i < list.size(); i++) {
                ome.model.core.Image img = list.get(i);

                /* test project/dataset membership */
                if ((dsImages != null) && (!dsImages.contains(img.getId()))) {
                    continue;
                }
                if ((prjImages != null) && (!prjImages.contains(img.getId()))) {
                    continue;
                }

                /* test comment/url/tag annotations */
                if ((urlMatches != null) && (!urlMatches.contains(img.getId()))) {
                    continue;
                }
                if ((commentMatches != null) && (!commentMatches.contains(img.getId()))) {
                    continue;
                }
                if ((tagMatches != null) && (!tagMatches.contains(img.getId()))) {
                    continue;
                }

                /* test name/description */
                if ((search.filename != null) && (!img.getName().contains(search.filename))) {
                    continue;
                }
                if ((search.description != null) && (!img.getDescription().contains(search.description))) {
                    continue;
                }
                
                /* test owner/group */
                if (search.owner != null) {
                    ome.model.meta.Experimenter owner = img.getDetails().getOwner();
                    owner = adminService.getExperimenter(owner.getId());
                    if (!search.owner.equals(owner.getOmeName())) {
                        continue;
                    }
                }
                if (search.group != null) {
                    ome.model.meta.ExperimenterGroup group = img.getDetails().getGroup();
                    group = adminService.getGroup(group.getId());
                    if (!search.group.equals(group.getName())) {
                        continue;
                    }
                }
                results.add(img.getName());
            }
        }
        
        return results;
    }

    // gets the metadata for a file
    public OMEROMetadata getMetadata(String filename) throws Exception {
        startup();

        OMEROMetadata result = new OMEROMetadata();

        result.comments = new Vector();
        result.tags = new Vector();
        result.links = new Vector();
        result.relatedFiles = new Vector();

        ome.model.core.Image img = queryService.findByString(ome.model.core.Image.class, "name", filename);

        // fill in basic info
        result.filename = img.getName();
        result.description = img.getDescription();
        
        // fill in experimenter and group names
        ome.model.meta.Experimenter owner = img.getDetails().getOwner();
        ome.model.meta.ExperimenterGroup group = img.getDetails().getGroup();

        owner = adminService.getExperimenter(owner.getId());
        result.owner = owner.getOmeName();

        group = adminService.getGroup(group.getId());
        result.group = group.getName();


        // fill in annotation stuff (comments, tags, links, related files)
        Set<Long> idset = new HashSet<Long>();
        idset.add(new Long(img.getId()));

        Map annotations = containerService.findAnnotations(ome.model.core.Image.class, idset, null, null);
        if (annotations != null) {
            Set annots = (Set)annotations.get(img.getId());
            if (annots != null) {
                Iterator iter = annots.iterator();
                while (iter.hasNext()) {
                    Object annobj = iter.next();
                    if (annobj instanceof ome.model.annotations.UrlAnnotation) {
                        pojos.URLAnnotationData uad = new pojos.URLAnnotationData((ome.model.annotations.UrlAnnotation) annobj);
                        result.links.add(uad.getURL());
                    }
                    else if (annobj instanceof ome.model.annotations.TagAnnotation) {
                        pojos.TagAnnotationData tad = new pojos.TagAnnotationData((ome.model.annotations.TagAnnotation) annobj);
                        result.tags.add(tad.getTagValue());
                    }
                    else if (annobj instanceof ome.model.annotations.FileAnnotation) {
                        pojos.FileAnnotationData fad = new pojos.FileAnnotationData((ome.model.annotations.FileAnnotation) annobj);
                        result.relatedFiles.add(new Long(fad.getId()));
                    }
                    else if (annobj instanceof ome.model.annotations.TextAnnotation) {
                        pojos.TextualAnnotationData tad = new pojos.TextualAnnotationData((ome.model.annotations.TextAnnotation) annobj);
                        result.comments.add(tad.getText());
                    }
                }
            }
        }

        // fill in project and dataset
        // must be a better way to do this but haven't found one
        List<ome.model.containers.Project> projects = queryService.findAll(ome.model.containers.Project.class, null);
        for (int i = 0; i < projects.size(); i++) {
            ome.model.containers.Project project = projects.get(i);
            Set<Long> idset2 = new HashSet<Long>();
            idset2.add(new Long(project.getId()));
            Set<ome.model.core.Image> imgset = containerService.getImages(ome.model.containers.Project.class, idset2, null);
            Iterator iter = imgset.iterator();
            while (iter.hasNext()) {
                ome.model.core.Image img2 = (ome.model.core.Image)iter.next();
                if (img2.getId().longValue() == img.getId().longValue()) {
                    result.project = project.getName();
                }
            }
        }

        List<ome.model.containers.Dataset> datasets = queryService.findAll(ome.model.containers.Dataset.class, null);
        for (int i = 0; i < datasets.size(); i++) {
            ome.model.containers.Dataset dataset = datasets.get(i);
            Set<Long> idset2 = new HashSet<Long>();
            idset2.add(new Long(dataset.getId()));
            Set<ome.model.core.Image> imgset = containerService.getImages(ome.model.containers.Dataset.class, idset2, null);
            Iterator iter = imgset.iterator();
            while (iter.hasNext()) {
                ome.model.core.Image img2 = (ome.model.core.Image)iter.next();
                if (img2.getId().longValue() == img.getId().longValue()) {
                    result.dataset = dataset.getName();
                }
            }
        }

        return result;
    }

    public Vector getProjects() throws Exception {
        startup();

        Vector result = new Vector();

        List<ome.model.containers.Project> list = queryService.findAll(ome.model.containers.Project.class, null);
        for (int i = 0; i < list.size(); i++) {
            ome.model.containers.Project prj = list.get(i);
            result.add(prj.getName());
        }

        return result;
    }

    public Vector getDatasets() throws Exception {
        startup();

        Vector result = new Vector();

        List<ome.model.containers.Dataset> list = queryService.findAll(ome.model.containers.Dataset.class, null);
        for (int i = 0; i < list.size(); i++) {
            ome.model.containers.Dataset ds = list.get(i);
            result.add(ds.getName());
        }

        return result;
    }

    public Vector getTags() throws Exception {
        startup();

        Vector result = new Vector();
        HashSet hs = new HashSet();

        Set<Long> idset = new HashSet<Long>();
        List<ome.model.core.Image> imglist = queryService.findAll(ome.model.core.Image.class, null);
        for (int i = 0; i < imglist.size(); i++) {
            ome.model.core.Image img = imglist.get(i);
            idset.add(new Long(img.getId()));
        }

        Map annotations = containerService.findAnnotations(ome.model.core.Image.class, idset, null, null);
        if (annotations != null) {
            Collection col = annotations.values();
            Iterator iter = col.iterator();
            while (iter.hasNext()) {
                Set annots = (Set)iter.next();
                if (annots != null) {
                    Iterator iter2 = annots.iterator();
                    while (iter2.hasNext()) {
                        Object annobj = iter2.next();
                        if (annobj instanceof ome.model.annotations.TagAnnotation) {
                            pojos.TagAnnotationData tad = new pojos.TagAnnotationData((ome.model.annotations.TagAnnotation) annobj);
                            hs.add(tad.getTagValue());
                        }
                    }
                }
            }
        }

        Iterator iter3 = hs.iterator();
        while (iter3.hasNext()) {
            result.add(iter3.next());
        }

        return result;
    }

    public Vector getUsers() throws Exception {
        startup();

        Vector result = new Vector();

        List<ome.model.meta.Experimenter> list = adminService.lookupExperimenters();
        for (int i = 0; i < list.size(); i++) {
            ome.model.meta.Experimenter exp = list.get(i);
            result.add(exp.getOmeName());
        }

        return result;
    }

    public Vector getGroups() throws Exception {
        startup();
        
        Vector result = new Vector();

        List<ome.model.meta.ExperimenterGroup> list = adminService.lookupGroups();
        for (int i = 0; i < list.size(); i++) {
            ome.model.meta.ExperimenterGroup group = list.get(i);
            result.add(group.getName());
        }

        return result;
    }

    /*
     * Sets the host where the OMERO server is installed
     */
    public void setHostname(String host) {
        hostname = host;
    }

    public static OMEROInterface getInstance() {
        if (instance == null) {
            instance = new OMEROInterface();
        }
        return instance;
    }
};
