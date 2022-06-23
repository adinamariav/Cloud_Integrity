import React, { Component } from "react";
import { Link } from 'react-router-dom'

class EducationalReport extends Component {
    render () {
        return(
            <div className="container-fluid mt-4 ml-3">
                <nav aria-label="..." style = {{ marginTop:`-1rem`}}>
                            <ul class="pagination pagination-lg">
                                <li class="page-item active" aria-current="page">
                                    <span class="page-link" style= {{ backgroundColor:`#D3E0EA`, color:`black`}}>View report</span>
                                </li>
                                <li class="page-item"><Link to="/educational/dashboard" class="page-link">Dashboard</Link></li>
                                <li class="page-item"><Link to="/educational/dashboard" class="page-link">Previous sessions</Link></li>
                            </ul>
                        </nav>
                <div className="card" style= {{ maxWidth:`98%` }}>
                    <div className="card-body" style = {{ backgroundColor: `#D3E0EA`}}>
                        <h1 style = {{ textAlign: `center` }}>Sessionz</h1>
                    </div>
                </div>
            </div>
        );
    }
}

export default EducationalReport;