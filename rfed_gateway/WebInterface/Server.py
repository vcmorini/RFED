import cherrypy
from cherrypy.lib import auth_digest
import os
import Database_handler as Dh
mongodb_handler = Dh.DatabaseHandler()

@cherrypy.expose
class WebInterfaceInterdisciplinary(object):
    def GET(self, *uri, **params):
        # Get the front page of your GUI
        if len(uri) == 0:
            return open("FrontPage.html")

    def POST(self, *uri, **params):
        # With the method "generate", insert a new patient when he arrives at the hospital, associating him with a MAC
        # address
        if uri[0] == "generate":
            try:
                r = mongodb_handler.generate(params["fname"], params["lname"], params["birth"], params["mac"])
                return open("PatientAdd.html")
            except (IndexError, ValueError, KeyError):
                return open("AddError.html")
        # Use method delete when you want to de-associate a patient with a MAC address, when the patient is no longer
        # in the health facility
        elif uri[0] == "delete":
            try:
                r = mongodb_handler.delete(params["fname"], params["lname"], params["birth"])
                return open("PatientDelete.html")
            except (IndexError, ValueError, KeyError):
                return open("AddError.html")


if __name__ == '__main__':
    # Users and passwords for the GUI
    USERS = {"Lorenzo": "groupg", "Victor": "groupg", "Cansu": "groupg", "Anastasya": "groupg"}
    cherrypy.config.update({'server.socket_host': '0.0.0.0'})
    cherrypy.config.update({'server.socket_port': 8080})
    PATH = os.path.abspath(os.getcwd())
    conf = {
        '/': {
            'request.dispatch': cherrypy.dispatch.MethodDispatcher(),
            'tools.staticdir.on': True,
            'tools.staticdir.dir': PATH,
            'tools.auth_digest.on': True,
            'tools.auth_digest.realm': 'localhost',
            'tools.auth_digest.get_ha1': auth_digest.get_ha1_dict_plain(USERS),
            'tools.auth_digest.key': 'a565c27146791cfb',
            'tools.auth_digest.accept_charset': 'UTF-8',
        }
    }
    cherrypy.tree.mount(WebInterfaceInterdisciplinary(), '/', conf)
    cherrypy.engine.start()
    cherrypy.engine.block()

