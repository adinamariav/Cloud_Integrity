import React from "react";
import {Link} from 'react-router-dom'

function Navbar() {
    return(
        <nav class="navbar navbar-expand-lg navbar-dark bg-dark shadow">
            <div class="container-fluid">
            <Link to="/" class="navbar-brand">Xen Intrusion-Detection System</Link>
                <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarSupportedContent" aria-controls="navbarSupportedContent" aria-expanded="false" aria-label="Toggle navigation">
                <span class="navbar-toggler-icon"></span>
                </button>
                <div class="collapse navbar-collapse" id="navbarSupportedContent">
                <ul class="navbar-nav me-auto mb-2 mb-lg-0">
                <li class="nav-item">
                    <Link to="/" class="nav-link active">Home</Link>
                    </li>
                    <li class="nav-item">
                    <Link to="/about" class="nav-link active">About</Link>
                    </li>
                    <li class="nav-item dropdown ml-auto">
                        <a class="nav-link dropdown-toggle" id="navbarDropdown" role="button" data-bs-toggle="dropdown" aria-expanded="false">
                            More options
                        </a>
                        <ul class="dropdown-menu" aria-labelledby="navbarDropdown">
                            <li><Link class="dropdown-item" to="/syscalls">View system calls</Link></li>
                        </ul>    
                    </li>
                </ul>
                </div>
            </div>
        </nav>
    );
}

export default Navbar;