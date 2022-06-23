import React from 'react';

import Home from './components/pages/Home';
import About from './components/pages/About';
import { DataProvider } from './components/includes/Context';
import Navbar from './components/includes/Navbar';
import Educational from './components/pages/Educational';
import Analyze from './components/pages/Analyze'
import Sandbox from './components/pages/Sandbox';
import EducationalSessions from './components/pages/EducationalSessions';
import EducationalReport from './components/pages/EducationalReport';

import { BrowserRouter as Router, Route, Routes } from 'react-router-dom'

function App() {
  return (
    <Router>
      <DataProvider>
        <div>
          <Navbar />
          <Routes>
            <Route path = "/" element={<Home />} />
            <Route path = "/about" element={<About />} />
            <Route path = "/educational/dashboard" element={<Educational />} />
            <Route path = "/analyze" element={<Analyze />} />
            <Route path = "/sandbox" element={<Sandbox />} />
            <Route path = "/educational/sessions" element={<EducationalSessions />} />
            <Route path = "/educational/report" element={<EducationalReport />} />

          </Routes>
        </div>
      </DataProvider>
  </Router>
      
  );
}

export default App;
