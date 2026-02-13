
warning C4828
You can open your file, which contain this warning, for ex. GameMode.cpp.
Then click File->AdvancedSavedOptions and choose Encoding Mode.
For us solution variant was “Unicode (UTF-8 with signature) - Codepage 65001”.
Hope this helps.






Connessione al DB
	ProgMaster_Fill2: Deve riempire una ListView con una tabella 
	ProgMaster_Insert: Crea una nuova riga nella tabella Master
	DBModifyCopyProcess: Rinomina o duplica una riga nella tabella Master;
		Se duplica, ricopia anche le righe della tabella Details

DBMaster:
Lista tutti i programmi nel database (tabella Master)
	Ordinamento per id o per nome
	Eventi interni
		Click Destro
			Cancellare/Rinominare/Copiare/Creare un nuovo programma
			Rimuovere i Details
			Stampare/Esportare
	Eventi per l'esterno
		Click sinistro su una riga da inviare alla Lista dei details
	Responsabilità
		Deve conoscere la Tabella Master
		Per Stampare/Esportare e rimuovere i Details deve conoscere la Tabella Details
		Deve potersi connettere a un DB
		
		DBCreateNewProcess();
		GetCurrRow
		DBModifyCopyProcess
		Reload
		ChangeSort
		MainPrg_Fill
	Files:
		PanelDBMaster.h e .cpp
		cMainListCtrl*		m_lstPrgMaster	= nullptr;

----------------------------------------
DBDetails
Lista dei Details
	Responsabilità
		Deve conoscere la Tabella Details
		Ordinamento fisso per Id
		Per l'inserimento deve poter accedere all'Editor dei comandi
	Eventi interni
		Spostare una riga in alto/basso
		Eliminare una riga
	Eventi per l'esterno
		Click su una riga
	Dall'esterno
		Rieseguire la lista per un determinato Master
---------------------------------------------
CmdEditorCtrl:  Editor dei comandi
	Responsabilità
		Deve conoscere quali comandi ci sono
		Quali e quanti parametri
	Richieste dall'esterno
		Deve posizionarsi su un comando specifico su richiesta
		Deve trasmettere il comando corrente a chi lo richiede
	...................................
	CmdParLabel: restituisce un editor per ciascun parametro
	...................................
	sSampler_Commands: è una struttura contenente lo schema di un'istruzione
------------------------------------------------
Master
	Quando si clicca su un master, ricostruisce i Details
	Se si Clicca su 
		"Esegui Tutto" Chiede al Details di volta in volta le righe e le esegue
		"Esegui Singolo" Chiede all'Editor dei comandi il comando corrente e lo esegue
		"Ferma tutto" smette di inviare i comandi
	