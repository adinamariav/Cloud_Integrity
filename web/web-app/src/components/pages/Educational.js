import React, { Component } from "react";
import VM from "../includes/VM.js"
import { DataContext } from "../includes/Context.js";
import SyscallTable from "../includes/SyscallTable.js";
import EducationalStatus from "../includes/EducationalStatus.js"
import Status from "../includes/Status.js";
import { Link } from 'react-router-dom'

class Educational extends Component {

    static contextType = DataContext;
    
    render () {
        return(
            <div className="container-fluid mt-4 ml-3">
                <nav aria-label="..." style = {{ marginTop:`-1rem`, marginLeft:`-1rem`}}>
                        <ul class="pagination">
                            <li class="page-item active" aria-current="page">
                            <span class="page-link" style= {{ backgroundColor:`#D3E0EA`, color:`black`}} >Dashboard</span>
                            </li>
                            <li class="page-item"><Link to="/educational/sessions" class="page-link">Previous sessions</Link></li>
                            <li class="page-item"><Link to="/educational/report" class="page-link">View report</Link></li>
                        </ul>
                    </nav>
                <div className="card" style= {{ maxWidth:`98%` }}>
                    <div className="row card-body" style = {{ backgroundColor: `#D3E0EA`, minHeight:`85vh`}}>
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