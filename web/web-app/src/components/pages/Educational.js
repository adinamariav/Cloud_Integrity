import React, { Component } from "react";
import VM from "../includes/VM.js"
import { DataContext } from "../includes/Context.js";
import SyscallTable from "../includes/SyscallTable.js";
import EducationalStatus from "../includes/EducationalStatus.js"
import Status from "../includes/Status.js";

class Educational extends Component {

    static contextType = DataContext;
    
    render () {
        return(
            <div className="container-fluid mt-4 ml-3">
                <div className="card" style= {{ maxWidth:`98%` }}>
                    <div className="row card-body" style = {{ backgroundColor: `#D3E0EA`, minHeight:`90vh`}}>
                        <div className="col center" style =  {{ display: 'flex', 
                                                                alignItems: 'center',
                                                                flexDirection: `col`,
                                                                justifyContent: 'center'}} >
                            <div className="row">
                                <VM />
                                <EducationalStatus />
                            </div>
                        </div>
                
                        <div className="col">
                        <Status />
                        <SyscallTable />
                           
                        </div>
                        
                    </div>
                </div>
            </div>
    );
    }
}

export default Educational;